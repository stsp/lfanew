dnl Copyright (c) 2018--2023 TK Chia
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions are
dnl met:
dnl
dnl   * Redistributions of source code must retain the above copyright
dnl     notice, this list of conditions and the following disclaimer.
dnl   * Redistributions in binary form must reproduce the above copyright
dnl     notice, this list of conditions and the following disclaimer in the
dnl     documentation and/or other materials provided with the distribution.
dnl   * Neither the name of the developer(s) nor the names of its
dnl     contributors may be used to endorse or promote products derived from
dnl     this software without specific prior written permission.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
dnl IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
dnl TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
dnl PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
dnl HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
dnl SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
dnl TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
dnl PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
dnl LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
dnl NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
dnl SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dnl Perform checks for underlying C library facilities.

AC_CHECK_DECL([O_TEXT],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE_O_TEXT],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([_O_TEXT],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE__O_TEXT],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([O_BINARY],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE_O_BINARY],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([_O_BINARY],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE__O_BINARY],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([_O_WTEXT],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE__O_WTEXT],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([_O_U8TEXT],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE__O_U8TEXT],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_DECL([_O_U16TEXT],
	      [AC_DEFINE([_LFANEW_INTERNAL_HAVE__O_U16TEXT],[1])],,
	      [#include <fcntl.h>])
AC_CHECK_FUNC([_setmode],[AC_DEFINE([_LFANEW_INTERNAL_HAVE__SETMODE],[1])])
AC_CHECK_HEADER([io.h],[AC_DEFINE([_LFANEW_INTERNAL_HAVE_IO_H],[1])])
AC_CHECK_FUNC([scandir],[AC_DEFINE([_LFANEW_INTERNAL_HAVE_SCANDIR],[1])])
AC_CHECK_MEMBER([struct dirent.d_namlen],dnl
  [AC_DEFINE([_LFANEW_INTERNAL_HAVE_STRUCT_DIRENT_D_NAMLEN],[1])],,dnl
  [#include <dirent.h>])
AC_CHECK_MEMBER([struct dirent.d_reclen],dnl
  [AC_DEFINE([_LFANEW_INTERNAL_HAVE_STRUCT_DIRENT_D_RECLEN],[1])],,dnl
  [#include <dirent.h>])

AC_SUBST(ac_cv_func_scandir)
