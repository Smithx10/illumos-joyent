#!/bin/ksh -p
#
# CDDL HEADER START
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#
# CDDL HEADER END
#

#
# Copyright (c) 2017 by Lawrence Livermore National Security, LLC.
# Copyright 2019 Joyent, Inc.
#

. $STF_SUITE/include/libtest.shlib
. $STF_SUITE/tests/functional/mmp/mmp.cfg

verify_runnable "global"

case "$(uname)" in
Linux)	if [ -e $HOSTID_FILE ]; then
		log_unsupported "System has existing $HOSTID_FILE file"
	fi
	log_must set_tunable64 zfs_multihost_history $MMP_HISTORY
	;;

SunOS)	hostid >/var/tmp/zfs_test_hostid.txt
	;;
esac

log_pass "mmp setup pass"
