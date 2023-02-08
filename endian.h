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

#ifndef H_ENDIAN
#define H_ENDIAN

#include <stdint.h>

#if (defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
     && __BYTE_ORDER - 0 == __ORDER_LITTLE_ENDIAN__ - 0) \
    || defined __i386__ || defined __i386 || defined __i86__ || defined __i86

typedef uint16_t uint_le16_t;
typedef uint32_t uint_le32_t;

static uint_le16_t
hle16 (uint16_t __x)
{
  return __x;
}

static uint_le32_t
hle32 (uint32_t __x)
{
  return __x;
}

static uint16_t
leh16 (uint_le16_t __x)
{
  return __x;
}

static uint32_t
leh32 (uint_le32_t __x)
{
  return __x;
}

#else  /* not known to be little endian */

typedef struct
  {
    uint8_t __octet[2];
    uint16_t : 0;
  } uint_le16_t;

typedef struct
  {
    uint8_t __octet[4];
    uint32_t : 0;
  } uint_le32_t;

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

#endif  /* not known to be little endian */
	
#endif
