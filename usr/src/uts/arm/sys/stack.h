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
 * Copyright 2013 (c) Joyent, Inc.  All rights reserved.
 */

#ifndef _SYS_STACK_H
#define	_SYS_STACK_H

/*
 * XXX It is required for sys/stack.h to require sys/types.h. However, at this
 * time we have not quite implemented all of the necessary mach related headers
 * for this. As such we comment this out for such a time.
 */
#if 0
#if !defined(_ASM)
#include <sys/types.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On ARM the stack ends up layed out as:
 *
 */

#define	STACK_ALIGN32		8
#define	STACK_ENTRY_ALIGN32	4
#define	STACK_BIAS32		0
#define	SA32(X)			(((X)+(STACK_ALIGN32-1)) & ~(STACK_ALIGN32-1))
#define	STACK_RESERVE32		0
#define	MINFRAME32		0

#define	STACK_ALIGN		STACK_ALIGN32
#define	STACK_ALIGN_ENTRY	STACK_ALIGN_ENTRY32
#define	STACK_BIAS		STACK_BIAS32
#define	SA(x)			SA32(x)
#define	STACK_RESERVE		STACK_RESERVE32
#define	MINFRAME		MINFRAME32

#define	STACK_GROWTH_DOWN /* stacks grow from high to low addresses */

#if defined(_KERNEL) && !defined(_ASM)
#if defined(DEBUG)
#define	ASSERT_STACK_ALIGNED()						\
	{								\
		uint32_t __tmp;						\
		ASSERT((((uintptr_t)&__tmp) & (STACK_ALIGN - 1)) == 0);	\
	}
#else /* DEBUG */
#define	ASSERT_STACK_ALIGNED()
#endif /* DEBUG */
#endif /* _KERNEL and !_ASM */

/* XXX the below typedef can be removed once we include <sys/types.h> */
typedef char *caddr_t;
struct regs;

void traceregs(struct regs *);
void traceback(caddr_t);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STACK_H */
