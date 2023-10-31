dnl Copyright (c) 2023 TK Chia
dnl
dnl This Source Code Form is subject to the terms of the Mozilla Public
dnl License, v. 2.0. If a copy of the MPL was not distributed with this
dnl file, You can obtain one at http://mozilla.org/MPL/2.0/.

_LFANEW_GCC_ACK_FIX

AC_PREREQ(2.69)
AC_INIT([lfanew],m4_esyscmd_s([
  if git diff --quiet HEAD 2>/dev/null; then
    TZ=UTC0 git log -n1 --oneline --date=short-local --format='%ad' | \
	    sed 's/-//g'
  else
    TZ=UTC0 date +%Y%m%d
  fi]),,[lfanew])
AC_CONFIG_SRCDIR([LICENSE])

AC_DISABLE_OPTION_CHECKING
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
lfanew_ackhost=
cflags_set="${CFLAGS+set}"
case "$host_alias" in
  i386-linuxack | i386-*-linuxack)
    lfanew_ackhost=linux386 ;;
  ia16-msdosack | ia16-*-msdosack)
    lfanew_ackhost=msdos86 ;;
  powerpc-linuxack | powerpc-*-linuxack)
    lfanew_ackhost=linuxppc ;;
esac
if test -n "$lfanew_ackhost"; then
  if test -z "$CC"; then
    AC_CHECK_PROG(CC, ack-cc, ack-cc -m"$lfanew_ackhost" -.c, dnl
			      ack -m"$lfanew_ackhost" -.c)
    export CC
  fi
  if test -z "$cflags_set"; then
    CFLAGS='-O6 -S'
    export CFLAGS
  fi
fi
AC_PROG_INSTALL
AC_PROG_GREP
AC_PROG_AWK
AC_LANG_WERROR
AC_PROG_CC
if test yes = "$GCC"; then
  CFLAGS="-Wall $CFLAGS"
elif test -z "$cflags_set"; then
  AC_CHECK_DECL([__KEFIRCC__],[CFLAGS='-O2'])
fi
AC_ARG_VAR(AR, [location of ar])
AC_ARG_VAR(RANLIB, [location of ranlib])
AC_ARG_VAR(LLVMCONFIG, [location of llvm-config])
AC_ARG_VAR(OBJCOPY, [location of llvm-objcopy])
AC_PATH_TOOL(AR, ar, /bin/false)
AC_PATH_TOOL(RANLIB, ranlib, :)
dnl GNU Binutils's objcopy cannot deal with Windows PE files produced by upx.
dnl LLVM's objcopy can.  So use that.  A complication is that only newer
dnl versions of llvm-objcopy support the --set-section-flags option for PE.
AC_CHECK_TOOL(LLVMCONFIG, [llvm-config], /bin/false)
case :"`$LLVMCONFIG --version 2>/dev/null`": in
  :[0-9].*: | :10.*: )
    AC_CHECK_TOOLS(OBJCOPY, [llvm-objcopy-14 llvm-objcopy-13 ]dnl
			    [llvm-objcopy-12 llvm-objcopy-11 ]dnl
			    [llvm-objcopy], /bin/false)
    ;;
  *)
    AC_CHECK_TOOLS(OBJCOPY, [llvm-objcopy llvm-objcopy-14 ]dnl
			    [llvm-objcopy-13 llvm-objcopy-12 ]dnl
			    [llvm-objcopy-11], /bin/false)
    ;;
esac
AC_ARG_VAR(UPX, [location of upx])
AC_PATH_PROG(UPX, upx, /bin/false)
case "$build" in
  *-cygwin* | *-mingw32* | *-mingw64* | *-msys* | *-windows* | *-winnt*)
    : ${WINRUN=}
    AC_CHECK_PROGS(WINCC,
		   [i686-mingw32-gcc mingw32-gcc ]dnl
		   [i686-w64-mingw32-gcc x86_64-w64-mingw32-gcc], "$CC")
    AC_CHECK_PROGS(WINCXX,
		   [i686-mingw32-g++ mingw32-g++ ]dnl
		   [i686-w64-mingw32-g++ x86_64-w64-mingw32-g++], "$CXX")
    ;;
  *)
    AC_PATH_PROG(WINRUN, wine, /bin/false)
    AC_CHECK_PROGS(WINCC,
		   [i686-mingw32-gcc mingw32-gcc ]dnl
		   [i686-w64-mingw32-gcc x86_64-w64-mingw32-gcc], /bin/false)
    AC_CHECK_PROGS(WINCXX,
		   [i686-mingw32-g++ mingw32-g++ ]dnl
		   [i686-w64-mingw32-g++ x86_64-w64-mingw32-g++], /bin/false)
    ;;
esac
case "$host" in
  *-cygwin* | *-mingw32* | *-mingw64* | *-msys* | *-windows* | *-winnt*)
    : ${HOSTRUN="$WINRUN"} ;;
  *)
    : ${HOSTRUN=} ;;
esac
if test x"$CC" = x"$WINCC"; then
  : ${WINCFLAGS="$CPPFLAGS $CFLAGS $LDFLAGS -s"}
  : ${WINLIBS="$LIBS"}
else
  : ${WINCFLAGS="-Wall -Os -s"}
  : ${WINLIBS=}
fi
AC_CACHE_CHECK(dnl
[whether $WINCC accepts -Wl,--section-alignment=],dnl
[lfanew_cv_prog_wincc_wl_section_alignment],[dnl
  AC_LANG_CONFTEST([AC_LANG_SOURCE([[int main (void) { return 0; }]])])
  [if $WINCC $WINCFLAGS -Wl,--section-alignment=0x8000 \
	    -o conftest.exe conftest.c $WINLIBS \
	    >&]AS_MESSAGE_LOG_FD[ 2>&1; then
    lfanew_cv_prog_wincc_wl_section_alignment=yes
  else
    lfanew_cv_prog_wincc_wl_section_alignment=no
  fi
  rm -f conftest.c conftest.exe]])
AC_CACHE_CHECK(dnl
[for patchable ldscripts/i386pe.x script for $WINCC],dnl
[lfanew_cv_prog_wincc_ldscripts_i386pe_x],[dnl
  [script=`$WINCC -print-file-name=ldscripts/i386pe.x`
  lfanew_cv_prog_wincc_ldscripts_i386pe_x=none
  case "$script" in
    ldscripts/i386pe.x)
      ;;
    *)
      if $GREP '^ *\.text  *__image_base__ + .* : *$' "$script" >/dev/null 2>&1
      then
        ]AC_LANG_CONFTEST([AC_LANG_SOURCE([[int main (void) { return 0; }]])])[
        if $WINCC $WINCFLAGS -T "$script" -o conftest.exe conftest.c $WINLIBS \
	   >&]AS_MESSAGE_LOG_FD[ 2>&1; then
	  lfanew_cv_prog_wincc_ldscripts_i386pe_x="$script"
	fi
      fi;;
  esac]])
AC_CACHE_CHECK(dnl
[for patchable ldscripts/i386pep.x script for $WINCC],dnl
[lfanew_cv_prog_wincc_ldscripts_i386pep_x],[dnl
  [script=`$WINCC -print-file-name=ldscripts/i386pep.x`
  lfanew_cv_prog_wincc_ldscripts_i386pep_x=none
  case "$script" in
    ldscripts/i386pep.x)
      ;;
    *)
      if $GREP '^ *\.text  *__image_base__ + .* : *$' "$script" >/dev/null 2>&1
      then
        ]AC_LANG_CONFTEST([AC_LANG_SOURCE([[int main (void) { return 0; }]])])[
        if $WINCC $WINCFLAGS -T "$script" -o conftest.exe conftest.c $WINLIBS \
	   >&]AS_MESSAGE_LOG_FD[ 2>&1; then
	  lfanew_cv_prog_wincc_ldscripts_i386pep_x="$script"
	fi
      fi;;
  esac]])
AC_CACHE_CHECK(dnl
[whether $WINCC accepts -Wl,-align:],[lfanew_cv_prog_wincc_wl_align],[dnl
  AC_LANG_CONFTEST([AC_LANG_SOURCE([[int main (void) { return 0; }]])])
  [if $WINCC $WINCFLAGS -Wl,-align:0x8000 -o conftest.exe conftest.c \
      >&]AS_MESSAGE_LOG_FD[ 2>&1; then
    lfanew_cv_prog_wincc_wl_align=yes
  else
    lfanew_cv_prog_wincc_wl_align=no
  fi
  rm -f conftest.c conftest.exe]])

m4_include([check-c.m4])

dnl These programs are only used when running tests, but look for them anyway.
AC_PATH_PROG(DOS2UNIX, dos2unix, /bin/false)
AC_PATH_PROG(NASM, nasm, /bin/false)
AC_PATH_PROGS(DOSRUN, [emu2 86sim msdos], /bin/false)

AC_SUBST([lfanew_ackhost])
AC_SUBST([WINRUN])
AC_SUBST([WINCC])
AC_SUBST([WINCXX])
AC_SUBST([WINCFLAGS])
AC_SUBST([WINLIBS])
AC_SUBST([HOSTRUN])
AC_SUBST([lfanew_cv_prog_wincc_wl_section_alignment])
AC_SUBST([lfanew_cv_prog_wincc_ldscripts_i386pe_x])
AC_SUBST([lfanew_cv_prog_wincc_ldscripts_i386pep_x])
AC_SUBST([lfanew_cv_prog_wincc_wl_align])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_FILES([Makefile msdospeify tests/atlocal],dnl
		[test -f msdospeify && chmod +x msdospeify])
AC_DEFINE_UNQUOTED([_LFANEW_VERSION],[${PACKAGE_VERSION}L])
AC_CONFIG_HEADERS([h/lfanew/_config.h h/lfanew/_version.h])
AC_OUTPUT
