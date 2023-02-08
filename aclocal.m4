dnl Copyright (c) 2020--2023 TK Chia
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

dnl Patch autoconf so that it recognizes an --ack option, & also works with
dnl the Amsterdam Compiler Kit's non-standard default output file names.
dnl	-- tkchia 20211030
dnl
dnl Also force autoconf _not_ to look for a C++ compiler, lest it decides to
dnl use the host g++.
dnl
dnl And, apparently there is a rather bad bug in autoconf 2.71 in the macros
dnl AC_TYPE_LONG_LONG_INT and AC_TYPE_UNSIGNED_LONG_LONG_INT: for some reason
dnl they assume that `long long' is supported if the C compiler only
dnl understands C89 (& C89 is the one C standard that does _not_ define a
dnl `long long'!).  Patch these macros to remove this bug.
dnl	-- tkchia 20220512
AC_DEFUN([_LFANEW_GCC_ACK_FIX],dnl
[m4_define([_LFANEW_SAVE_AC_INIT_PARSE_ARGS],dnl
m4_defn([_AC_INIT_PARSE_ARGS]))dnl
m4_define([_AC_INIT_PARSE_ARGS],dnl
[m4_bpatsubst(m4_defn([_LFANEW_SAVE_AC_INIT_PARSE_ARGS]),dnl
[--x)],dnl
[-ack | --ack)
    host_alias=ia16-pc-msdosack
    if test NONE = "[$]exec_prefix"; then
      exec_prefix='[$]{prefix}'/share/ack/'[$]{libi86_ackhost}'
    fi
    if test '[$]{exec_prefix}/lib' = "$libdir"; then  # FIXME?
      libdir='[$]{exec_prefix}'
    fi
    CXX=/bin/false ;;
 --x)])])dnl
m4_define([_LFANEW_SAVE_AC_COMPILER_EXEEXT_DEFAULT],dnl
m4_defn([_AC_COMPILER_EXEEXT_DEFAULT]))dnl
m4_define([_AC_COMPILER_EXEEXT_DEFAULT],dnl
[m4_bpatsubst(m4_defn([_LFANEW_SAVE_AC_COMPILER_EXEEXT_DEFAULT]),dnl
[a_out\.exe],dnl
[a_out.exe cpm.com e.out linux386.exe linux68k.exe linuxmips.exe ]dnl
[linuxppc.exe msdos86.exe msdos86.com msdos386.exe osx386.exe osxppc.exe ]dnl
[pc86.img qemuppc.img raspberrypi.bin])])dnl
m4_define([_LFANEW_SAVE_AC_TYPE_UNSIGNED_LONG_LONG_INT],dnl
m4_defn([AC_TYPE_UNSIGNED_LONG_LONG_INT]))dnl
m4_defun([AC_TYPE_UNSIGNED_LONG_LONG_INT],dnl
[m4_bpatsubst(m4_defn([_LFANEW_SAVE_AC_TYPE_UNSIGNED_LONG_LONG_INT]),dnl
[no | c89) ;;],dnl
[lolwut) ;;])])dnl
m4_define([_LFANEW_SAVE_AC_TYPE_LONG_LONG_INT],dnl
m4_defn([AC_TYPE_LONG_LONG_INT]))dnl
m4_defun([AC_TYPE_LONG_LONG_INT],dnl
[m4_bpatsubst(m4_defn([_LFANEW_SAVE_AC_TYPE_LONG_LONG_INT]),dnl
[no | c89) ;;],dnl
[lolwut) ;;])])])
