dnl
dnl Copyright (c) 2015-2019 Cray Inc. All rights reserved.
dnl Copyright (c) 2015-2018 Los Alamos National Security, LLC.
dnl                         All rights reserved.
dnl
dnl This software is available to you under a choice of one of two
dnl licenses.  You may choose to be licensed under the terms of the GNU
dnl General Public License (GPL) Version 2, available from the file
dnl COPYING in the main directory of this source tree, or the
dnl BSD license below:
dnl
dnl     Redistribution and use in source and binary forms, with or
dnl     without modification, are permitted provided that the following
dnl     conditions are met:
dnl
dnl      - Redistributions of source code must retain the above
dnl        copyright notice, this list of conditions and the following
dnl        disclaimer.
dnl
dnl      - Redistributions in binary form must reproduce the above
dnl        copyright notice, this list of conditions and the following
dnl        disclaimer in the documentation and/or other materials
dnl        provided with the distribution.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
dnl "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
dnl LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
dnl FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
dnl COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
dnl INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
dnl BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
dnl LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
dnl CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
dnl ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
dnl POSSIBILITY OF SUCH DAMAGE.
dnl

dnl Configury specific to the libfabrics GNI provider

dnl Called to configure this provider
dnl SST/macro version adapted from Cray

m4_include([config/fi_pkg.m4])

AC_DEFUN([FI_SSTMAC_CONFIGURE],[
  AC_SUBST(sstmac_CPPFLAGS)
  AC_SUBST(sstmac_LDFLAGS)
	AC_SUBST(sstmactest_CPPFLAGS)
  AC_SUBST(sstmactest_LDFLAGS)
  AC_SUBST(sstmactest_LIBS)
	$1
])
