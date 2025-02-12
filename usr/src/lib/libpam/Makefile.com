#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

LIBRARY=	 libpam.a
VERS=		.1
OBJECTS=	pam_framework.o

include		../../Makefile.lib
include		../../Makefile.rootfs

TEXT_DOMAIN=	SUNW_OST_SYSOSPAM

LIBS=		$(DYNLIB) $(LINTLIB)
$(LINTLIB) :=	SRCS= $(SRCDIR)/$(LINTSRC)
LDLIBS +=	-lc

CFLAGS +=	$(CCVERBOSE)
CPPFLAGS +=	-I $(SRCDIR)

CERRWARN +=	$(CNOWARN_UNINIT)

.KEEP_STATE:

all:		$(LIBS)

lint:		lintcheck

include		../../Makefile.targ
