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
 * Copyright (c) 2018, Joyent, Inc.
 */

/*
 * Routines for parsing flattened device tree blob that the bootloader
 * passes us. This contains information like reserved zones of memory,
 * available, CPUS available, uart address, etc.
 */

#include <sys/types.h>
// #include <sys/param.h>
#include <sys/fdt.h>
#include <sys/byteorder.h>

// extern void bcopy(const void *, void *, size_t);

uintptr_t
fdt_mem_resvmap_addr(fdt_header_t *hdr)
{
	return (uintptr_t) hdr + ntohl(hdr->fdt_off_mem_rsvmap);
}

uintptr_t
fdt_strings_addr(fdt_header_t *hdr)
{
	return (uintptr_t) hdr + ntohl(hdr->fdt_off_dt_strings);
}

uintptr_t
fdt_struct_addr(fdt_header_t *hdr)
{
	return (uintptr_t) hdr + ntohl(hdr->fdt_off_dt_struct);
}

char *
fdt_string_at_off(fdt_header_t *hdr, uint32_t off)
{
	uintptr_t str_addr = fdt_strings_addr(hdr);
	return (char *)(str_addr + off);
}


// atag_header_t *
// atag_next(atag_header_t *cur)
// {
// 	uintptr_t addr;

// 	addr = (uintptr_t)cur;
// 	addr += cur->ah_size * 4;
// 	cur = (atag_header_t *)addr;
// 	if (cur->ah_tag == ATAG_NONE)
// 		return (NULL);

// 	return (cur);
// }

// const atag_header_t *
// atag_find(atag_header_t *start, uint32_t type)
// {
// 	while (start != NULL) {
// 		if (start->ah_tag == type)
// 			return (start);
// 		start = atag_next(start);
// 	}

// 	return (NULL);
// }

// /*
//  * Append an atag header to the end of the chain. Note that this will clobber
//  * memory.
//  */
// void
// atag_append(atag_header_t *chain, atag_header_t *val)
// {
// 	atag_header_t hdr, *next;

// 	while (chain->ah_tag != ATAG_NONE) {
// 		next = atag_next(chain);
// 		if (next == NULL) {
// 			chain = (atag_header_t *)
// 			    (((uintptr_t)chain) + chain->ah_size * 4);
// 		} else {
// 			chain = next;
// 		}
// 	}

// 	hdr.ah_size = chain->ah_size;
// 	hdr.ah_tag = chain->ah_tag;

// 	(void) bcopy(val, chain, val->ah_size * 4);
// 	chain = (atag_header_t *)(((uintptr_t)chain) + val->ah_size * 4);
// 	chain->ah_size = hdr.ah_size;
// 	chain->ah_tag = hdr.ah_tag;
// }

// size_t
// atag_length(atag_header_t *chain)
// {
// 	size_t len = 0;
// 	while (chain != NULL) {
// 		len += chain->ah_size * 4;
// 		chain = atag_next(chain);
// 	}
// 	/* Add in the zero tag */
// 	len += sizeof (atag_header_t);

// 	return (len);
// }
