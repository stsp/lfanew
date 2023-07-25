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

#ifndef _LFANEW_NEXGEN_MZENDIAN_H
#define _LFANEW_NEXGEN_MZENDIAN_H

#include <stdint.h>

typedef struct
  {
    uint8_t __octet[2];
  } uint_le16_t;

typedef struct
  {
    uint8_t __octet[4];
  } uint_le32_t;

typedef struct
  {
    uint8_t __octet[8];
  } uint_le64_t;

typedef union
  {
    uint_le16_t __xo;
    uint16_t __xi;
  } uint_lea16_t;

typedef union
  {
    uint_le32_t __xo;
    uint_le16_t __xw[2];
    uint32_t __xi;
  } uint_lea32_t;

typedef union
  {
    uint_le64_t __xo;
    uint_le32_t __xdw[2];
#ifdef UINT64_MAX
    uint64_t __xi;
#endif
  } uint_lea64_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#undef _NEXGEN_MZENDIAN_ASSUME_LE
#if ! defined __ACK || defined __OPTIMIZE__ \
    || defined __SW_OI || defined __SW_OR  /* -oi or -or under Open Watcom */
# if defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
     && __BYTE_ORDER__ - 0 == __ORDER_LITTLE_ENDIAN__ - 0
#   define _NEXGEN_MZENDIAN_ASSUME_LE
# elif defined __i386__ || defined __i386 || defined __386__ \
       || defined __i86__ || defined __i86 || defined __I86__ \
       || defined __ARMEL__ || defined __AARCH64EL__
#   define _NEXGEN_MZENDIAN_ASSUME_LE
# endif
#endif

#ifdef _NEXGEN_MZENDIAN_ASSUME_LE

static uint_le16_t
hle16 (uint16_t __x)
{
  uint_lea16_t __u;
  __u.__xi = __x;
  return __u.__xo;
}

static uint_le32_t
hle32 (uint32_t __x)
{
  uint_lea32_t __u;
  __u.__xi = __x;
  return __u.__xo;
}

# ifdef UINT64_MAX
static uint_le64_t
hle64 (uint64_t __x)
{
  uint_lea64_t __u;
  __u.__xi = __x;
  return __u.__xo;
}
# endif

static uint16_t
leh16 (uint_le16_t __x)
{
  uint_lea16_t __u;
  __u.__xo = __x;
  return __u.__xi;
}

static uint32_t
leh32 (uint_le32_t __x)
{
  uint_lea32_t __u;
  __u.__xo = __x;
  return __u.__xi;
}

static uint16_t
leh32lo (uint_le32_t __x)
{
  uint_lea32_t __u;
  __u.__xo = __x;
  return leh16 (__u.__xw[0]);
}

static uint16_t
leh32hi (uint_le32_t __x)
{
  uint_lea32_t __u;
  __u.__xo = __x;
  return leh16 (__u.__xw[1]);
}

# ifdef UINT64_MAX
static uint64_t
leh64 (uint_le64_t __x)
{
  uint_lea64_t __u;
  __u.__xo = __x;
  return __u.__xi;
}
# endif

static uint32_t
leh64lo (uint_le64_t __x)
{
  uint_lea64_t __u;
  __u.__xo = __x;
  return leh32 (__u.__xdw[0]);
}

static uint32_t
leh64hi (uint_le64_t __x)
{
  uint_lea64_t __u;
  __u.__xo = __x;
  return leh32 (__u.__xdw[1]);
}

#else  /* not known to be little endian */

static uint_le16_t
hle16 (uint16_t __x)
{
  uint_le16_t __xle;
  __xle.__octet[0] = (uint8_t)  __x;
  __xle.__octet[1] = (uint8_t) (__x >> 8);
  return __xle;
}

static uint_le32_t
hle32 (uint32_t __x)
{
  uint_le32_t __xle;
  __xle.__octet[0] = (uint8_t)  __x;
  __xle.__octet[1] = (uint8_t) (__x >>  8);
  __xle.__octet[2] = (uint8_t) (__x >> 16);
  __xle.__octet[3] = (uint8_t) (__x >> 24);
  return __xle;
}

# ifdef UINT64_MAX
static uint_le64_t
hle64 (uint64_t __x)
{
  uint_le64_t __xle;
  __xle.__octet[0] = (uint8_t)  __x;
  __xle.__octet[1] = (uint8_t) (__x >>  8);
  __xle.__octet[2] = (uint8_t) (__x >> 16);
  __xle.__octet[3] = (uint8_t) (__x >> 24);
  __xle.__octet[4] = (uint8_t) (__x >> 32);
  __xle.__octet[5] = (uint8_t) (__x >> 40);
  __xle.__octet[6] = (uint8_t) (__x >> 48);
  __xle.__octet[7] = (uint8_t) (__x >> 56);
  return __xle;
}
# endif

static uint16_t
leh16 (uint_le16_t __x)
{
  return (uint16_t) __x.__octet[1] << 8
		  | __x.__octet[0];
}

static uint32_t
leh32 (uint_le32_t __x)
{
  return   (uint32_t) __x.__octet[3] << 24
	 | (uint32_t) __x.__octet[2] << 16
	 | (uint32_t) __x.__octet[1] <<  8
	 |	      __x.__octet[0];
}

static uint16_t
leh32lo (uint_le32_t __x)
{
  return (uint16_t) __x.__octet[1] <<  8
		  | __x.__octet[0];
}

static uint16_t
leh32hi (uint_le32_t __x)
{
  return (uint16_t) __x.__octet[3] <<  8
		  | __x.__octet[2];
}

# ifdef UINT64_MAX
static uint64_t
leh64 (uint_le64_t __x)
{
  return   (uint64_t) __x.__octet[7] << 56
	 | (uint64_t) __x.__octet[6] << 48
	 | (uint64_t) __x.__octet[5] << 40
	 | (uint64_t) __x.__octet[4] << 32
	 | (uint64_t) __x.__octet[3] << 24
	 | (uint64_t) __x.__octet[2] << 16
	 | (uint64_t) __x.__octet[1] <<  8
	 |	      __x.__octet[0];
}
# endif

static uint32_t
leh64lo (uint_le64_t __x)
{
  return   (uint32_t) __x.__octet[3] << 24
	 | (uint32_t) __x.__octet[2] << 16
	 | (uint32_t) __x.__octet[1] <<  8
	 |	      __x.__octet[0];
}

static uint32_t
leh64hi (uint_le64_t __x)
{
  return   (uint32_t) __x.__octet[7] << 24
	 | (uint32_t) __x.__octet[6] << 16
	 | (uint32_t) __x.__octet[5] <<  8
	 |	      __x.__octet[4];
}

#endif  /* not known to be little endian */

#pragma GCC diagnostic pop

#endif
