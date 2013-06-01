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

#ifndef _BMC2835_MINIUART_H
#define	_BMC2835_MINIUART_H

/*
 * Interface to the BMC2835's miniuart.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

void bmc2835_miniuart_init(void);
void bmc2835_miniuart_putc(uint8_t);
uint8_t bmc2835_miniuart_getc(void);

#ifdef __cplusplus
}
#endif

#endif /* _BMC2835_MINIUART_H */
