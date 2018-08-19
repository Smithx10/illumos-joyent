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
 * Copyright 2013 (c) Joyent, Inc. All rights reserved.
 */

/*
 * This file contains various ARM subroutines that are generic across all ARM
 * platforms.
 */

#include <sys/asm_linkage.h>

#if defined(__lint)
#include <sys/thread.h>
#endif /* __lint */

#if defined(__lint)

/*
 * Return the current kernel thread that's running. Note that this is also
 * available as an inline function.
 */
kthread_id_t
threadp(void)
{ return ((kthread_id_t)0); }

#else	/* __lint */

	ENTRY(threadp)
	mrs x0, tpidr_el0
	ret
	SET_SIZE(threadp)

#endif	/* __lint */
