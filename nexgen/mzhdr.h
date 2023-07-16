/*
 * Copyright (c) 2023 TK Chia
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the developer(s) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NEXGEN_MZHDR_H
#define _NEXGEN_MZHDR_H

#include <nexgen/mzendian.h>

typedef struct
  {
    uint_le16_t e_magic;
    uint_le16_t e_cblp;
    uint_le16_t e_cp;
    uint_le16_t e_crlc;
    uint_le16_t e_cparhdr;
    uint_le16_t e_minalloc;
    uint_le16_t e_maxalloc;
    uint_le16_t e_ss;
    uint_le16_t e_sp;
    uint_le16_t e_csum;
    uint_le16_t e_ip;
    uint_le16_t e_cs;
    uint_le16_t e_lfarlc;
    uint_le16_t e_ovno;
    uint_le16_t e_res[16];
    uint_le32_t e_lfanew;
  } mz_hdr_t;

#define MZ_MAGIC	0x5a4dU
#define MZ_PG_SZ	0x200U
#define MZ_PARA_SZ	0x10U
#define MZ_RELOC_SZ	4U

#define MZ_LFARLC_NEW	(sizeof (mz_hdr_t))

#endif
