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
case "$OS" in
  windows)
    # Download & install nasm.  This follows https://github.com/vim/vim-win32
    # -installer/blob/507d8257c76afa87e2b753b46ae3cd479f9d1f0b/appveyor.bat .
    curl -f -L https://cygwin.com/setup-x86_64.exe -o ./setup-x86_64.exe
    chmod +x ./setup-x86_64.exe
    ./setup-x86_64.exe -qnNdO -P nasm
    # Download & install Takeda's MS-DOS Player, takeda-toshiya.my.coocan.jp/
    # msdos/ .  The caller script is expected to add the correct directory to
    # the %PATH%.
    curl -f -L http://takeda-toshiya.my.coocan.jp/msdos/msdos.7z -o ./msdos.7z
    7z x ./msdos.7z;;
  *)
    apt-get update -y
    apt-get install -y software-properties-common
    # add-apt-repository may sometimes time out trying to download the PPA's
    # public key.
    add-apt-repository -y ppa:tkchia/de-rebus \
     || apt-key add tests/ppa-pub-key.gpg.bin
    apt-get update -y
    apt-get install -y dos2unix nasm emu2.dmsc autoconf make wine;;
esac
