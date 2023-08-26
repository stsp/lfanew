#!/bin/sh
# Copyright (c) 2018--2023 TK Chia
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the developer(s) nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Script to install dependency packages for automated building under AppVeyor
# CI (https://ci.appveyor.com/), invoked by .appveyor.yml .

set -e -v
AUTOCONF="$*"
case "$OS" in
  windows)
    # Unpack nasm, Takeda's MS-DOS Player, & upx.  Download these if
    # necessary.  The caller script is expected to add the correct
    # directories to %PATH%.
    pkg=nasm-2.16.01-win32.zip
    if test \! -f ./3rd-party/"$pkg"; then
      curl -f -L https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win32/` \
		 `"$pkg" -o ./3rd-party/"$pkg"
    fi
    7z x ./3rd-party/"$pkg"
    pkg=msdos.7z
    if test \! -f ./3rd-party/"$pkg"; then
      curl -f -L http://takeda-toshiya.my.coocan.jp/msdos/"$pkg" \
	   -o ./3rd-party/"$pkg"
    fi
    7z x ./3rd-party/"$pkg"
    ver=4.1.0
    pkg=upx-"$ver"-win32.zip
    if test \! -f ./3rd-party/"$pkg"; then
      curl -f -L https://github.com/upx/upx/releases/download/"$ver"/"$pkg" \
	   -o ./3rd-party/"$pkg"
    fi
    7z x ./3rd-party/"$pkg";;
  *)
    apt-get install -y software-properties-common
    # add-apt-repository may sometimes time out trying to download the PPA's
    # public key.
    add-apt-repository -y ppa:tkchia/de-rebus \
     || apt-key add tests/ppa-pub-key.gpg.bin
    apt-get update -y
    set -- dos2unix nasm emu2.dmsc autoconf make wine gcc-mingw-w64-i686 \
	   g++-mingw-w64-i686 p7zip-full upx-ucl llvm-14
    case "$AUTOCONF:$CC" in
      *chibicc*)
	set -- chibicc "$@";;
    esac
    apt-get install -y "$@";;
esac
