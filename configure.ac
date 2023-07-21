dnl Copyright (c) 2023 TK Chia
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
  if test -z "$CFLAGS"; then
    CFLAGS='-O6 -S'
    export CFLAGS
  fi
fi
AC_PROG_INSTALL
AC_LANG_WERROR
AC_PROG_CC
if test yes = "$GCC"; then
  CFLAGS="-Wall $CFLAGS"
fi

AC_SUBST([lfanew_ackhost])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
