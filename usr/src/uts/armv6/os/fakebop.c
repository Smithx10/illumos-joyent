/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright (c) 2013 Joyent, Inc.  All rights reserved.
 */

/*
 * Just like in i86pc, we too get the joys of mimicking the SPARC boot system.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bootconf.h>
#include <sys/bootsvcs.h>
#include <sys/boot_console.h>
#include <sys/atag.h>
#include <sys/varargs.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/ctype.h>

static bootops_t bootop;
static uint_t kbm_debug = 1;
#define	DBG_MSG(x)	{ if (kbm_debug) bcons_puts(x); bcons_puts("\n\r"); }
#define	BUFFERSIZE	256
static char buffer[BUFFERSIZE];

/*
 * Fakebop Memory allocation Scheme
 *
 * Early on during boot we need to be able to allocate memory before we get the
 * normal kernel memory allocator up and running. Currently this memory is never
 * freed by the system, or so it appears. As such we take a much simpler
 * approach to the memory allocation. Mostly, we care off a region of memory
 * from the top of our physical range equal to bop_alloc_nfree which is
 * currently 16 MB. From there, allocations are given a 4-byte aligned chunks of
 * memory from bop_alloc_start until we run out of memory. Because of the lack
 * of any callers of the memory free routing in the system, we do not bother
 * with anything like a free list or even size and next pointers. This does make
 * the debugging a bit harder, but hopefully our use of this will be minimal
 * overall.
 *
 * TODO On other platforms this often includes using virtual memory instead of
 * physical memory. While it would be great to do that on ARM, let's start with
 * this strawman approach and later revamp it. The goal here is that it's meant
 * to be very simple... albeit this will probably be too simple in time to come.
 */
#define	FAKEBOP_ALLOC_SIZE	(16 * 1024 * 1024)
static uintptr_t bop_alloc_start = 0;
static uintptr_t bop_alloc_nfree = FAKEBOP_ALLOC_SIZE;

#define	BI_HAS_RAMDISK	0x1

/*
 * TODO Generalize this
 * This is the set of information tha we want to gather from the various atag
 * headers. This is simple and naive and will need to evolve as we have
 * additional boards beyond just the RPi.
 */
typedef struct bootinfo {
	uint_t		bi_flags;
	uint32_t	bi_memsize;
	uint32_t	bi_memstart;
	char 		*bi_cmdline;
	uint32_t	bi_ramdisk;
	uint32_t	bi_ramsize;
} bootinfo_t;

static bootinfo_t bootinfo;	/* Simple set of boot information */

static struct boot_syscalls bop_sysp = {
	bcons_getchar,
	bcons_putchar,
	bcons_ischar,
};

/*
 * stuff to store/report/manipulate boot property settings.
 */
typedef struct bootprop {
	struct bootprop *bp_next;
	char *bp_name;
	uint_t bp_vlen;
	char *bp_value;
} bootprop_t;

static bootprop_t *bprops = NULL;

void
bop_panic(const char *msg)
{
	bop_printf(NULL, "ARM bop_panic:\n\r%s\n\rSpinning Forever...", msg);
	for (;;)
		;
}

/*
 * XXX This is just a hack to let us see a bit more about what's going on.
 * Normally we'd use vsnprintf, but that includes sys/systm.h which requires
 * almost every header platform header in the world. Also, we're using hex,
 * because hex is cool. Actually, we're really using it because it means we can
 * bitshift instead of divide. There is no integer division in ARMv6 natively.
 * Oops.
 */
static char *
fakebop_hack_ultostr(unsigned long value, char *ptr)
{
	ulong_t t, val = (ulong_t)value;
	char c;

	do {
		c = (char)('0' + val - 16 * (t = (val >> 4)));
		if (c > '9')
			c += 'A' - '9' - 1;
		*--ptr = c;
	} while ((val = t) != 0);

	*--ptr = 'x';
	*--ptr = '0';

	return (ptr);
}

/*
 * We need to reserve bop_alloc_nfree bytes from the top of our memory range. We
 * also need to make sure that nothing else is using that region of memory, eg.
 * the initial ram disk or the kernel.
 */
static void
fakebop_alloc_init(void)
{
	uintptr_t top;

	top = bootinfo.bi_memstart + bootinfo.bi_memsize;
	top -= bop_alloc_nfree;

	bop_printf(NULL, "bot_alloc_nfree: %d\n\r", bop_alloc_nfree);
	if (top > bootinfo.bi_ramdisk &&
	    top < bootinfo.bi_ramdisk + bootinfo.bi_ramsize)
		bop_panic("fakebop_alloc_init memory range has overlaps");
	bop_alloc_start = top;
	bop_printf(NULL, "starting with memory at 0x%x and have %d bytes\n\r",
	    top, bop_alloc_nfree);
}

static void
fakebop_dump_tags(void *tagstart)
{
	atag_header_t *h = tagstart;
	atag_core_t *acp;
	atag_mem_t *amp;
	atag_cmdline_t *alp;
	atag_initrd_t *aip;
	const char *tname;
	int i;
	char *c;

	DBG_MSG("starting point:");
	DBG_MSG(fakebop_hack_ultostr((uintptr_t)h, &buffer[BUFFERSIZE-1]));
	DBG_MSG("first atag size:");
	DBG_MSG(fakebop_hack_ultostr(h->ah_size, &buffer[BUFFERSIZE-1]));
	DBG_MSG("first atag tag:");
	DBG_MSG(fakebop_hack_ultostr(h->ah_tag, &buffer[BUFFERSIZE-1]));
	while (h != NULL) {
		switch (h->ah_tag) {
		case ATAG_CORE:
			tname = "ATAG_CORE";
			break;
		case ATAG_MEM:
			tname = "ATAG_MEM";
			break;
		case ATAG_VIDEOTEXT:
			tname = "ATAG_VIDEOTEXT";
			break;
		case ATAG_RAMDISK:
			tname = "ATAG_RAMDISK";
			break;
		case ATAG_INITRD2:
			tname = "ATAG_INITRD2";
			break;
		case ATAG_SERIAL:
			tname = "ATAG_SERIAL";
			break;
		case ATAG_REVISION:
			tname = "ATAG_REVISION";
			break;
		case ATAG_VIDEOLFB:
			tname = "ATAG_VIDEOLFB";
			break;
		case ATAG_CMDLINE:
			tname = "ATAG_CMDLINE";
			break;
		default:
			tname = fakebop_hack_ultostr(h->ah_tag,
			    &buffer[BUFFERSIZE-1]);
			break;
		}
		DBG_MSG("tag:");
		DBG_MSG(tname);
		DBG_MSG("size:");
		DBG_MSG(fakebop_hack_ultostr(h->ah_size,
		    &buffer[BUFFERSIZE-1]));
		/* Extended information */
		switch (h->ah_tag) {
		case ATAG_CORE:
			if (h->ah_size == 2) {
				DBG_MSG("ATAG_CORE has no extra information");
			} else {
				acp = (atag_core_t *)h;
				DBG_MSG("\tflags:");
				bcons_puts("\t");
				DBG_MSG(fakebop_hack_ultostr(acp->ac_flags,
				    &buffer[BUFFERSIZE-1]));
				DBG_MSG("\tpage:");
				bcons_puts("\t");
				DBG_MSG(fakebop_hack_ultostr(acp->ac_pagesize,
				    &buffer[BUFFERSIZE-1]));
				DBG_MSG("\troot:");
				bcons_puts("\t");
				DBG_MSG(fakebop_hack_ultostr(acp->ac_rootdev,
				    &buffer[BUFFERSIZE-1]));
			}
			break;
		case ATAG_MEM:
			amp = (atag_mem_t *)h;
			DBG_MSG("\tsize:");
			bcons_puts("\t");
			DBG_MSG(fakebop_hack_ultostr(amp->am_size,
			    &buffer[BUFFERSIZE-1]));
			DBG_MSG("\tstart:");
			bcons_puts("\t");
			DBG_MSG(fakebop_hack_ultostr(amp->am_start,
			    &buffer[BUFFERSIZE-1]));
			break;
		case ATAG_INITRD2:
			aip = (atag_initrd_t *)h;
			DBG_MSG("\tsize:");
			bcons_puts("\t");
			DBG_MSG(fakebop_hack_ultostr(aip->ai_size,
			    &buffer[BUFFERSIZE-1]));
			DBG_MSG("\tstart:");
			bcons_puts("\t");
			DBG_MSG(fakebop_hack_ultostr(aip->ai_start,
			    &buffer[BUFFERSIZE-1]));
			break;
		case ATAG_CMDLINE:
			alp = (atag_cmdline_t *)h;
			DBG_MSG("\tcmdline:");
			/*
			 * We have no intelligent thing to wrap our tty at 80
			 * chars so we just do this a bit more manually for now.
			 */
			i = 0;
			c = alp->al_cmdline;
			while (*c != '\0') {
				bcons_putchar(*c++);
				if (++i == 72) {
					bcons_puts("\n\r");
					i = 0;
				}
			}
			bcons_puts("\n\r");
			break;
		default:
			break;
		}
		h = atag_next(h);
	}
}

static void
fakebop_getatags(void *tagstart)
{
	atag_mem_t *amp;
	atag_cmdline_t *alp;
	atag_header_t *ahp = tagstart;
	atag_initrd_t *aip;
	bootinfo_t *bp = &bootinfo;

	bp->bi_flags = 0;
	while (ahp != NULL) {
		switch (ahp->ah_tag) {
		case ATAG_MEM:
			amp = (atag_mem_t *)ahp;
			bp->bi_memsize = amp->am_size;
			bp->bi_memstart = amp->am_start;
			break;
		case ATAG_CMDLINE:
			alp = (atag_cmdline_t *)ahp;
			bp->bi_cmdline = alp->al_cmdline;
		case ATAG_INITRD2:
			aip = (atag_initrd_t *)ahp;
			bp->bi_ramdisk = aip->ai_start;
			bp->bi_ramsize = aip->ai_size;
			bp->bi_flags |= BI_HAS_RAMDISK;
		default:
			break;
		}
		ahp = atag_next(ahp);
	}
}

static caddr_t
fakebop_alloc(struct bootops *bops, caddr_t virthint, size_t size, int align)
{
	caddr_t start;

	bop_printf(bops, "Asked to allocated %d bytes\n\r", size);
	if (bop_alloc_start == 0)
		bop_panic("fakebop_alloc_init not called");

	if (align == BO_ALIGN_DONTCARE)
		align = 4;

	size = P2ROUNDUP(size, align);
	bop_printf(bops, "Asked to allocated %d bytes %d free\n\r", size,
	    bop_alloc_nfree);
	if (size > bop_alloc_nfree)
		bop_panic("fakebop_alloc ran out of memory");

	start = (caddr_t)bop_alloc_start;
	bop_alloc_start += size;
	bop_alloc_nfree -= size;
	return (start);
}

static void
fakebop_free(struct bootops *bops, caddr_t virt, size_t size)
{
	bop_panic("Called into fakebop_free");
}

static int
fakebop_getproplen(struct bootops *bops, const char *prop)
{
	bop_panic("Called into fakebop_getproplen");
	return (-1);
}

static int
fakebop_getprop(struct bootops *bops, const char *prop, void *value)
{
	bop_panic("Called into fakebop_getprop");
	return (-1);
}

void
bop_printf(bootops_t *bop, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) vsnprintf(buffer, BUFFERSIZE, fmt, ap);
	va_end(ap);
	bcons_puts(buffer);
}

void
boot_prop_finish(void)
{
	bop_panic("Called into boot_prop_finish");
}

static void
fakebop_setprop(char *name, int nlen, void *value, int vlen)
{
	size_t size;
	bootprop_t *bp;
	caddr_t cur;

	size = sizeof (bootprop_t) + nlen + 1 + vlen;
	cur = fakebop_alloc(NULL, NULL, size, BO_ALIGN_DONTCARE);
	bp = (bootprop_t *)cur;
	if (bprops == NULL) {
		bprops = bp;
	} else {
		bp->bp_next = bprops;
		bprops = bp;
	}
	cur += sizeof (bootprop_t);
	bp->bp_name = cur;
	bcopy(name, cur, nlen);
	cur += nlen;
	*cur = '\0';
	cur++;
	bp->bp_value = cur;
	if (vlen > 0)
		bcopy(value, cur, vlen);
}

static void
fakebop_setprop_string(char *name, char *value)
{
	fakebop_setprop(name, strlen(name), value, strlen(value) + 1);
}

static void
fakebop_setprop_64(char *name, uint64_t value)
{
	fakebop_setprop(name, strlen(name), (void *)&value, sizeof (value));
}

/*
 * Here we create a bunch of the initial boot properties. This includes what
 * we've been passed in via the command line. It also includes a few values that
 * we compute ourselves.
 */
static void
fakebop_bootprops_init(void)
{
	int i = 0;
	char *c, *kernel;
	bootinfo_t *bp = &bootinfo;

	/*
	 * Set the ramdisk properties for kobj_boot_mountroot() can succeed.
	 */
	if ((bp->bi_flags & BI_HAS_RAMDISK) != 0) {
		fakebop_setprop_64("ramdisk_start",
		    (uint64_t)(uintptr_t)bp->bi_ramdisk);
		fakebop_setprop_64("ramdisk_end",
		    (uint64_t)(uintptr_t)bp->bi_ramdisk + bp->bi_ramsize);
	}

	/*
	 * Our boot parameters always wil start with kernel /platform/..., but
	 * the bootloader may in fact stick other properties in front of us. To
	 * deal with that we go ahead and we include them. We'll find the kernel
	 * and our properties set via -B when we finally find something without
	 * an equals sign, mainly kernel.
	 */
	c = bp->bi_cmdline;
	kernel = strstr(c, "kernel");
	if (kernel == NULL)
		bop_panic("failed to find kernel string in boot params!");
}

/*
 * Welcome to the kernel. We need to make a fake version of the boot_ops and the
 * boot_syscalls and then jump our way to _kobj_boot(). Here, we're borrowing
 * the Linux bootloader expectations, mostly because a lot of bootloaders and
 * boards already do this. If it turns out that we want to abstract this in the
 * future, then we should have locore.s do that before we get here.
 */
void
_fakebop_start(void *zeros, uint32_t machid, void *tagstart)
{
	bootinfo_t *bip = &bootinfo;
	bootops_t *bops = &bootop;
	extern void _kobj_boot();

	/*
	 * TODO Turn on caches and unaligned access!
	 */

	fakebop_getatags(tagstart);
	bcons_init(bip->bi_cmdline);

	bop_printf(NULL, "\n\rWelcome to fakebop -- ARM edition\n\r");
	fakebop_dump_tags(tagstart);

	/*
	 * Fill in the bootops vector
	 */
	bops->bsys_version = BO_VERSION;
	bops->bsys_alloc = fakebop_alloc;
	bops->bsys_free = fakebop_free;
	bops->bsys_getproplen = fakebop_getproplen;
	bops->bsys_getprop = fakebop_getprop;
	bops->bsys_printf = bop_printf;

	fakebop_alloc_init();
	fakebop_bootprops_init();
	_kobj_boot(&bop_sysp, NULL, bops);

	bop_panic("Returned from kobj_init\n");
}

void
_fakebop_locore_start(struct boot_syscalls *sysp, struct bootops *bops)
{
	bop_panic("Somehow made it back to fakebop_locore_start...");
}
