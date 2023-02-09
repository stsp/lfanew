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

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mz.h"

#ifdef __ACK
extern int getopt (int, char * const[], const char *);
extern char *optarg;
extern int optind;
#endif

typedef unsigned ui_t;
typedef unsigned long ul_t;

static bool keep_output = false;
static const char *me = NULL, *out_path = NULL, *in_path = NULL;
static FILE *in = NULL, *out = NULL;

static void
usage (void)
{
  fprintf (stderr, "usage: %s [-k] -o OUT-STUB-FILE IN-STUB-FILE\n", me);
  exit (1);
}

static void
clean_up (void)
{
  if (out)
    fclose (out);
  if (in)
    fclose (in);
  if (! keep_output && out_path)
   remove (out_path);
}

static void
error (const char *fmt, ...)
{
  va_list ap;
  clean_up ();
  fprintf (stderr, "%s: error: ", me);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
  exit (2);
}

static void
error_with_errno (const char *fmt, ...)
{
  va_list ap;
  int err = errno;
  clean_up ();
  fprintf (stderr, "%s: error: ", me);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ": %s\n", strerror (err));
  exit (2);
}

static void
warn (const char *fmt, ...)
{
  va_list ap;
  fprintf (stderr, "%s: warning: ", me);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
}

static void
parse_args (int argc, char **argv)
{
  int opt;
  char *ep;
  while ((opt = getopt (argc, argv, "a:ko:")) != -1)
    {
      switch (opt)
	{
	case 'k':
	  keep_output = true;
	  break;
	case 'o':
	  out_path = optarg;
	  break;
	default:
	  usage ();
	}
    }
  if (!out_path || optind != argc - 1)
    usage ();
  in_path = argv[optind];
}

static void
copy (FILE *in, FILE *out, uint32_t sz, uint32_t in_off, uint32_t out_off)
{
  char buf[BUFSIZ];
  while (sz)
    {
      size_t n = sz < BUFSIZ ? sz : BUFSIZ;
      if (fread (buf, 1, n, in) != n)
	error_with_errno ("cannot read input file at offset %#lx",
			  (ul_t) in_off);
      in_off += n;
      if (fwrite (buf, 1, n, out) != n)
	error_with_errno ("cannot copy to output file at offset %#lx",
			  (ul_t) out_off);
      out_off += n;
      sz -= n;
    }
}

static void
pad (FILE *out, size_t n)
{
  char buf = 0;
  while (n-- != 0)
    if (fwrite (&buf, 1, 1, out) != 1)
      error_with_errno ("cannot pad output file");
}

static void
lfanew (void)
{
  ul_t tot_sz, aligned_tot_sz;
  mz_hdr_t mz;
  uint16_t lfarlc, oem, new_lfarlc, cparhdr, crlc, cp, cblp;
  uint32_t mz_sz, new_mz_sz, new_tot_sz, rels_sz, rels_end, hdr_end,
	   new_rels_end, new_hdr_end;

  in = fopen (in_path, "rb");
  if (! in)
    error_with_errno ("cannot open input file");

  if (fseek (in, 0L, SEEK_END) != 0)
    error_with_errno ("cannot get size of input file");

  tot_sz = (ul_t) (ftell (in) + 1);
  if (! tot_sz)
    error_with_errno ("cannot get size of input file");
  --tot_sz;

  if (tot_sz > (ul_t) UINT32_MAX - MZ_PARA_SZ + 1)
    error ("input file too large, %#lx > 4 GiB - %#x",
	   tot_sz, (ui_t) MZ_PARA_SZ);
  aligned_tot_sz = (tot_sz + MZ_PARA_SZ - 1) & -(ul_t) MZ_PARA_SZ;

  memset (&mz, 0, sizeof mz);

  if (tot_sz < offsetof (mz_hdr_t, e_res)
      || fseek (in, 0L, SEEK_SET) != 0)
    error ("input is not MZ file");
  if (fread (&mz, offsetof (mz_hdr_t, e_res), 1, in) != 1)
    error_with_errno ("cannot read input file");
  if (leh16 (mz.e_magic) != MZ_MAGIC)
    error ("input is not MZ file");

  lfarlc = leh16 (mz.e_lfarlc);
  new_lfarlc = sizeof mz;
  if (lfarlc < offsetof (mz_hdr_t, e_res))
    error ("malformed MZ, e_lfarlc = %#x < %#x",
	   (ui_t) lfarlc, (ui_t) offsetof (mz_hdr_t, e_res));
  if (lfarlc >= new_lfarlc)
    error ("cannot rewrite MZ, e_lfarlc = %#x >= %#x",
	   (ui_t) lfarlc, (ui_t) new_lfarlc);

  cp = leh16 (mz.e_cp);
  cblp = leh16 (mz.e_cblp);
  if (! cblp)
    mz_sz = (long) cp * MZ_PG_SZ;
  else if (! cp)
    error ("malformed MZ, e_cp = 0 but e_cblp = %#x > 0", (ui_t) cblp);
  else
    {
      mz_sz = (long) (cp - 1) * MZ_PG_SZ + cblp;
      if (cblp >= MZ_PG_SZ)
	warn ("e_cblp == %#x > %#x looks bogus", (ui_t) cblp, (ui_t) MZ_PG_SZ);
    }

  cparhdr = leh16 (mz.e_cparhdr);
  hdr_end = cparhdr * (uint32_t) MZ_PARA_SZ;
  if (hdr_end > mz_sz)
    error ("MZ header overshoots MZ end, %#lx > %#lx",
	   (ul_t) hdr_end, (ul_t) mz_sz);
  if (hdr_end > tot_sz)
    error ("MZ header overshoots file end, %#lx > %#lx",
	   (ul_t) hdr_end, tot_sz);

  crlc = leh16 (mz.e_crlc);
  rels_sz = (uint32_t) crlc * MZ_RELOC_SZ;
  rels_end = lfarlc + (uint32_t) crlc * MZ_RELOC_SZ;
  if (rels_end > hdr_end)
    error ("MZ relocations overshoot MZ header end, %#x + %#lx > %#lx",
	   (ui_t) lfarlc, (ul_t) (rels_end - lfarlc), (ul_t) hdr_end);

  oem = lfarlc - offsetof (mz_hdr_t, e_res);
  if (oem != 0 && fread (&mz.e_res, oem, 1, in) != 1)
    error ("cannot read optional header information");

  new_rels_end = new_lfarlc + (uint32_t) crlc * MZ_RELOC_SZ;
  new_hdr_end = (new_rels_end + MZ_PARA_SZ - 1) & -(uint32_t) MZ_PARA_SZ;
  new_mz_sz = mz_sz - hdr_end + new_hdr_end;
  if (mz_sz == tot_sz)
    {
      new_mz_sz = aligned_tot_sz - hdr_end + new_hdr_end;  /* N.B. */
      new_tot_sz = new_mz_sz;
    }
  else
    {
      new_mz_sz = mz_sz - hdr_end + new_hdr_end;
      if (aligned_tot_sz >= mz_sz
	  && aligned_tot_sz - mz_sz > (ul_t) UINT32_MAX - new_mz_sz)
	error ("output file will be too large, %#lx - %#lx + %#lx > 4 GiB - 1",
	       aligned_tot_sz, (ul_t) mz_sz, (ul_t) new_mz_sz);
      new_tot_sz = aligned_tot_sz - mz_sz + new_mz_sz;
    }

  mz.e_cblp = hle16 (new_mz_sz % MZ_PG_SZ);
  mz.e_cp = hle16 ((new_mz_sz + MZ_PG_SZ - 1) / MZ_PG_SZ);
  mz.e_cparhdr = hle16 (new_hdr_end / MZ_PARA_SZ);
  mz.e_lfarlc = hle16 (new_lfarlc);
  mz.e_lfanew = hle32 (new_tot_sz);

  out = fopen (out_path, "wb");
  if (! out)
    error_with_errno ("cannot open output file");

  if (fwrite (&mz, sizeof mz, 1, out) != 1)
    error_with_errno ("cannot write output file");

  copy (in, out, rels_sz, lfarlc, new_lfarlc);
  pad (out, new_hdr_end - new_rels_end);
  copy (in, out, tot_sz - hdr_end, hdr_end, new_hdr_end);
  pad (out, aligned_tot_sz - tot_sz);

  if (fclose (out) != 0)
    error_with_errno ("cannot close output file");
  fclose (in);
}

static const char *
basenm (const char *path)
{
  const char *p = path, *q = path;
  char c;
  while ((c = *p++) != 0)
    if (c == '/')
      q = p;
  return q;
}

int
main (int argc, char **argv)
{
  me = basenm (argv[0]);
  parse_args (argc, argv);
  lfanew ();
  return 0;
}
