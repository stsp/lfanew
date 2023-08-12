#!/usr/bin/awk -f
# Copyright (c) 2023 TK Chia
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Script to patch the GNU linker scripts for PE/i386 and PE/x86-64 to allow
# the starting RVA of the .text section to be customized.

BEGIN {
  print "/****** AUTOMATICALLY PATCHED ******/"
}

/^ *\.text  *__image_base__ \+ .* : *$/ && ! /__base_of_code__/ {
  slack_def = $0
  sub (/^ *\.text  */, "  __lfanew_slack_base = ", slack_def)
  sub (/: *$/, ";", slack_def)
  print slack_def
  print "  __base_of_code__ = DEFINED (__base_of_code__)"
  print "		      ? __base_of_code__ : __lfanew_slack_base;"
  print "  __base_of_code__ = ALIGN (__base_of_code__, __section_alignment__);"
  print "  .slack __lfanew_slack_base :"
  print "  {"
  print "    . += __image_base__ + __base_of_code__ - __lfanew_slack_base;"
  print "  }"
  print "  .text __image_base__ + __base_of_code__ :"
  next
}

{
  print
}
