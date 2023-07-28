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
#include <nexgen/mzendian.h>
#include <nexgen/mzhdr.h>
#include <nexgen/pehdr.h>

#ifdef __ACK
extern int getopt (int, char * const[], const char *);
extern char *optarg;
extern int optind;
#endif

typedef unsigned ui_t;
typedef unsigned long ul_t;

typedef enum
{
  MODE_LFANEW,
  MODE_STUBIFY,
  MODE_UNSTUBIFY,
  MODE_DEFAULT = MODE_LFANEW
} op_mode_t;

static op_mode_t op_mode = MODE_DEFAULT;
static bool keep_output = false, force_cp_cblp = false;
static const char *me = NULL, *out_path = NULL,
		  *in1_path = NULL, *in2_path = NULL;
static FILE *in1 = NULL, *in2 = NULL, *out = NULL;

static void
usage (void)
{
  fprintf
    (stderr, "usage:\n"
	     "  # to add .e_lfanew\n"
	     "  %s [-k] -o OUT-STUB-FILE IN-STUB-FILE\n"
	     "  # to stubify\n"
	     "  %s -S [-k] [-p] -o OUT-FAT-FILE IN-PAYLOAD-FILE IN-STUB-FILE\n"
	     "  # to unstubify\n"
	     "  %s -U [-k] -o OUT-PAYLOAD-FILE IN-FAT-FILE\n",
     me, me, me);
  exit (1);
}

static void
clean_up (void)
{
  if (out)
    fclose (out);
  if (! keep_output && out_path)
   remove (out_path);
  if (in1)
    fclose (in1);
  if (in2)
    fclose (in2);
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
  while ((opt = getopt (argc, argv, "ko:pSU")) != -1)
    {
      switch (opt)
	{
	case 'k':
	  keep_output = true;
	  break;
	case 'o':
	  out_path = optarg;
	  break;
	case 'p':
	  force_cp_cblp = true;
	  break;
	case 'S':
	  op_mode = MODE_STUBIFY;
	  break;
	case 'U':
	  op_mode = MODE_UNSTUBIFY;
	  break;
	default:
	  usage ();
	}
    }
  if (! out_path)
    usage ();
  switch (op_mode)
    {
    case MODE_STUBIFY:
      if (optind != argc - 2)
	usage ();
      in2_path = argv[optind + 1];
      break;
    default:
      if (optind != argc - 1)
	usage ();
    }
  in1_path = argv[optind];
}

static FILE *
open_in (const char *path)
{
  FILE *in = fopen (path, "rb");
  if (! in)
    error_with_errno ("cannot open input file");
  return in;
}

static FILE *
open_out (const char *path)
{
  FILE *out = fopen (path, "wb");
  if (! out)
    error_with_errno ("cannot open output file");
  return out;
}

static void
close_out (FILE *out)
{
  if (fclose (out) != 0)
    error_with_errno ("cannot close output file");
}

static ul_t
size_it (FILE *in)
{
  fpos_t pos;
  ul_t tot_sz;

  if (fgetpos (in, &pos) != 0
      || fseek (in, 0L, SEEK_END) != 0)
    error_with_errno ("cannot get size of input file");

  tot_sz = (ul_t) (ftell (in) + 1);
  if (! tot_sz)
    error_with_errno ("cannot get size of input file");
  --tot_sz;

  if (fsetpos (in, &pos) != 0)
    error_with_errno ("cannot get size of input file");

  return tot_sz;
}

static void
process_mz_hdr_common (FILE *in, mz_hdr_t *pmz, ul_t *p_tot_sz,
		       uint32_t *p_mz_sz)
{
  ul_t tot_sz;
  uint16_t lfarlc, cp, cblp, cparhdr;
  uint32_t mz_sz, hdr_end;

  tot_sz = size_it (in);
  if (tot_sz < offsetof (mz_hdr_t, e_res))
    error ("input is not MZ file");

  memset (pmz, 0, sizeof (mz_hdr_t));
  if (fread (pmz, offsetof (mz_hdr_t, e_res), 1, in) != 1)
    error_with_errno ("cannot read input file");
  if (leh16 (pmz->e_magic) != MZ_MAGIC)
    error ("input is not MZ file");

  lfarlc = leh16 (pmz->e_lfarlc);
  if (lfarlc < offsetof (mz_hdr_t, e_res))
    error ("malformed MZ, e_lfarlc = %#x < %#x",
	   (ui_t) lfarlc, (ui_t) offsetof (mz_hdr_t, e_res));

  cp = leh16 (pmz->e_cp);
  cblp = leh16 (pmz->e_cblp);
  if (! cp)
    error ("malformed MZ, e_cp = 0");
  if (! cblp)
    mz_sz = (long) cp * MZ_PG_SZ;
  else
    {
      mz_sz = (long) (cp - 1) * MZ_PG_SZ + cblp;
      if (cblp >= MZ_PG_SZ)
	warn ("e_cblp == %#x > %#x looks bogus", (ui_t) cblp, (ui_t) MZ_PG_SZ);
    }

  cparhdr = leh16 (pmz->e_cparhdr);
  hdr_end = cparhdr * (uint32_t) MZ_PARA_SZ;
  if (hdr_end > mz_sz)
    error ("MZ header overshoots MZ end, %#lx > %#lx",
	   (ul_t) hdr_end, (ul_t) mz_sz);
  if (hdr_end > tot_sz)
    error ("MZ header overshoots file end, %#lx > %#lx",
	   (ul_t) hdr_end, tot_sz);
  if (lfarlc > hdr_end)
    error ("malformed MZ, e_lfarlc = %#x > %#lx",
	   (ui_t) lfarlc, (ul_t) hdr_end);

  *p_tot_sz = tot_sz;
  *p_mz_sz = mz_sz;
}

static void
process_ne_magic (FILE *in, uint_le32_t *p_ne_magic_le, const char *what)
{
  size_t magic_len = sizeof (*p_ne_magic_le);
  uint16_t ne_magic_lo;
  if (fread (p_ne_magic_le, 1, magic_len, in) != magic_len)
    error ("cannot read payload magic number");
  ne_magic_lo = leh32lo (*p_ne_magic_le);
  switch (ne_magic_lo)
    {
    default:
      break;
    case 0x454c:  /* "LE" */
    case 0x584c:  /* "LX" */
      /*
       * The LE, LX, & PE file format structures contain file offsets that
       * are relative to the beginning of the entire input file.  To truly
       * unstubify these file formats, we will have to adjust these offsets
       * correctly, & maybe even realign the file contents.  Currently
       * lfanew only knows how to do this for PE files.
       */
      error ("I do not know how to %s an LE or LX program", what);
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
skip (FILE *in, size_t n)
{
  char buf;
  while (n-- != 0)
    if (fread (&buf, 1, 1, in) != 1)
      error_with_errno ("cannot skip input bytes");
}

static void
copy (FILE *in, FILE *out, ul_t sz, uint32_t in_off, uint32_t out_off)
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

static bool
is_two_power (ul_t value)
{
  return value != 0 && (value & (value - 1)) == 0;
}

static void
pe_adjust_off (uint_le32_t *p_off, uint32_t aligned_inh_size, uint32_t adjust)
{
  uint32_t off = leh32 (*p_off);
  if (off != 0)
    {
      if (off < aligned_inh_size)
	error ("cannot adjust PE file pointer %#lx < %#lx",
	       (ul_t) off, (ul_t) aligned_inh_size);
      *p_off = hle32 (off + adjust);
    }
}

static void
copy_pe (FILE *in, FILE *out, ul_t sz, uint32_t in_off, uint32_t out_off)
{
  uint32_t inh_size, aligned_inh_size, outh_size, aligned_outh_size,
	   inh_slack, outh_slack, file_align, adjust, code_base, data_base;
  pe_img_fil_hdr_t fhdr;
  pe_img_opt_hdr_t ophdr;
  uint16_t ophdr_size, ophdr_magic, num_sects, i;
  pe_data_dir_t *ent;
  pe_sect_hdr_t sect;

  if (sizeof (fhdr) > sz)
    error ("PE COFF file header overshoots file end");

  if (fread (&fhdr, sizeof (fhdr), 1, in) != 1)
    error_with_errno ("cannot read PE COFF file header");

  if (leh32 (fhdr.PointerToSymbolTable) != 0
      || leh32 (fhdr.NumberOfSymbols) != 0)
    error ("PE COFF symbol table unsupported");

  if (fwrite (&fhdr, sizeof (fhdr), 1, out) != 1)
    error_with_errno ("cannot write PE COFF file header");

  in_off += sizeof (fhdr);
  out_off += sizeof (fhdr);
  sz -= sizeof (fhdr);

  ophdr_size = leh16 (fhdr.SizeOfOptionalHeader);
  if (ophdr_size < sizeof (ophdr.h.Magic))
    error ("PE optional header too short");
  if (ophdr_size > sizeof (ophdr))
    error ("long PE optional header unsupported, %#x > %#x",
	   (ui_t) ophdr_size, (ui_t) sizeof (ophdr));
  if (ophdr_size > sz)
    error ("PE optional header overshoots file end");

  memset (&ophdr, 0, sizeof (ophdr));
  if (fread (&ophdr, ophdr_size, 1, in) != 1)
    error_with_errno ("cannot read PE optional header");

  file_align = leh32 (ophdr.h.FileAlignment);
  if (! is_two_power (file_align))
    error ("PE FileAlignment %#lx not power of 2", (ul_t) file_align);
  num_sects = leh16 (fhdr.NumberOfSections);

  inh_size = (uint32_t) ophdr_size + (uint32_t) num_sects * sizeof (sect);
  if (inh_size > sz)
    error ("PE section headers overshoot file end");

  outh_size = inh_size + out_off;
  inh_size += in_off;
  aligned_inh_size = leh32 (ophdr.h.SizeOfHeaders);
  if (inh_size > aligned_inh_size)
    error ("PE SizeOfHeaders is bogus, %#lx > %#lx",
	   (ul_t) inh_size, (ul_t) aligned_inh_size);
  if (aligned_inh_size - in_off > sz)
    error ("PE SizeOfHeaders overshoots file end, %#lx > %#lx",
	   (ul_t) (aligned_inh_size - in_off), (ul_t) sz);

  aligned_outh_size = (outh_size + file_align - 1) & -file_align;
  if (aligned_outh_size < outh_size)
    error ("output PE headers will be too large, %#lx + %#lx > 4 GiB",
	   (ul_t) outh_size, (ul_t) file_align);
  ophdr.h.SizeOfHeaders = hle32 (aligned_outh_size);
  adjust = aligned_outh_size - aligned_inh_size;

  /*
   * We need enough virtual address space between {ImageBase} & {ImageBase
   * + BaseOfCode} to accommodate all the headers, i.e.  MZ stub, PE headers,
   * & section headers.  For now, simply error out if the BaseOfCode RVA is
   * too small.  (Alternatively, we could try to adjust all the RVAs in the
   * program upwards, but this is hard to do right & not a lot of fun.)
   */
  code_base = leh32 (ophdr.h.BaseOfCode);
  if (code_base < aligned_outh_size)
    error ("PE BaseOfCode too small for output PE headers, %#lx < %#lx",
	   (ul_t) code_base, (ul_t) aligned_outh_size);

  ophdr_magic = leh16 (ophdr.h.Magic);
  switch (ophdr_magic)
    {
    default:
      error ("PE optional header type %#x unsupported", (ui_t) ophdr_magic);

    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:  /* PE32 */
      if (ophdr_size > sizeof (ophdr.h32))
	error ("long PE32 header unsupported, %#x > %#x",
	       (ui_t) ophdr_size, (ui_t) sizeof (ophdr.h32));

      data_base = leh32 (ophdr.h32.BaseOfData);
      if (data_base < aligned_outh_size)
	error ("PE32 BaseOfData too small for output PE headers: %#lx < %#lx",
	       (ul_t) data_base, (ul_t) aligned_outh_size);

      ent = &ophdr.h32.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
      if (leh32 (ent->VirtualAddress) != 0 || leh32 (ent->Size) != 0)
	error ("PE32 debug table unsupported");

      ent = &ophdr.h32.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
      pe_adjust_off (&ent->VirtualAddress, aligned_inh_size, adjust);
      break;

    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:  /* PE32+ */
      ent = &ophdr.h64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
      if (leh32 (ent->VirtualAddress) != 0 || leh32 (ent->Size) != 0)
	error ("PE32 debug table unsupported");

      ent = &ophdr.h64.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
      pe_adjust_off (&ent->VirtualAddress, aligned_inh_size, adjust);
    }

  if (fwrite (&ophdr, ophdr_size, 1, out) != 1)
    error_with_errno ("cannot write PE optional header");

  in_off += ophdr_size;
  out_off += ophdr_size;
  sz -= ophdr_size;

  i = 0;
  while (i != num_sects)
    {
      ++i;

      if (sizeof (sect) > sz)
	error ("PE section header #%#x overshoots file end", (ui_t) i);

      if (fread (&sect, sizeof (sect), 1, in) != 1)
	error_with_errno ("cannot read PE section header #%#x", (ui_t) i);

      pe_adjust_off (&sect.PointerToRawData, aligned_inh_size, adjust);
      pe_adjust_off (&sect.PointerToRelocations, aligned_inh_size, adjust);
      pe_adjust_off (&sect.PointerToLinenumbers, aligned_inh_size, adjust);

      if (fwrite (&sect, sizeof (sect), 1, out) != 1)
	error_with_errno ("cannot write PE section header #%#x", (ui_t) i);

      in_off += sizeof (sect);
      out_off += sizeof (sect);
      sz -= sizeof (sect);
    }

  inh_slack = aligned_inh_size - inh_size;
  outh_slack = aligned_outh_size - outh_size;
  if (inh_slack < outh_slack)
    {
      copy (in, out, inh_slack, in_off, out_off);
      pad (out, outh_slack - inh_slack);
    }
  else
    {
      copy (in, out, outh_slack, in_off, out_off);
      skip (in, inh_slack - outh_slack);
    }

  in_off += inh_slack;
  out_off += outh_slack;
  sz -= inh_slack;
  copy (in, out, sz, in_off, out_off);
}

static void
copy_ne (FILE *in, FILE *out, ul_t sz, uint32_t in_off, uint32_t out_off,
	 uint_le32_t ne_magic_le)
{
  switch (leh32 (ne_magic_le))
    {
    default:
      copy (in, out, sz, in_off, out_off);
      break;

    case 0x00004550:  /* "PE", (?) little endian */
      copy_pe (in, out, sz, in_off, out_off);
    }
}

static void
lfanew (void)
{
  ul_t tot_sz, aligned_tot_sz;
  mz_hdr_t mz;
  uint16_t lfarlc, oem, cparhdr, crlc;
  uint32_t mz_sz, new_mz_sz, new_tot_sz, rels_sz, rels_end, hdr_end,
	   new_rels_end, new_hdr_end;

  in1 = open_in (in1_path);
  process_mz_hdr_common (in1, &mz, &tot_sz, &mz_sz);

  if (tot_sz > (ul_t) UINT32_MAX - MZ_PARA_SZ + 1)
    error ("input file too large, %#lx > 4 GiB - %#x",
	   tot_sz, (ui_t) MZ_PARA_SZ);
  aligned_tot_sz = (tot_sz + MZ_PARA_SZ - 1) & -(ul_t) MZ_PARA_SZ;

  lfarlc = leh16 (mz.e_lfarlc);
  if (lfarlc >= MZ_LFARLC_NEW - sizeof (uint32_t))
    error ("cannot rewrite MZ, e_lfarlc = %#x >= %#x",
	   (ui_t) lfarlc, (ui_t) (MZ_LFARLC_NEW - sizeof (uint32_t)));

  cparhdr = leh16 (mz.e_cparhdr);
  hdr_end = cparhdr * (uint32_t) MZ_PARA_SZ;

  crlc = leh16 (mz.e_crlc);
  rels_sz = (uint32_t) crlc * MZ_RELOC_SZ;
  rels_end = lfarlc + (uint32_t) crlc * MZ_RELOC_SZ;
  if (rels_end > hdr_end)
    error ("MZ relocations overshoot MZ header end, %#x + %#lx > %#lx",
	   (ui_t) lfarlc, (ul_t) (rels_end - lfarlc), (ul_t) hdr_end);

  oem = lfarlc - offsetof (mz_hdr_t, e_res);
  if (oem != 0 && fread (mz.e_res, oem, 1, in1) != 1)
    error ("cannot read optional header information");

  new_rels_end = MZ_LFARLC_NEW + (uint32_t) crlc * MZ_RELOC_SZ;
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
  mz.e_lfarlc = hle16 (MZ_LFARLC_NEW);
  mz.e_lfanew = hle32 (new_tot_sz);

  out = open_out (out_path);
  if (fwrite (&mz, sizeof mz, 1, out) != 1)
    error_with_errno ("cannot write output file");

  copy (in1, out, rels_sz, lfarlc, MZ_LFARLC_NEW);
  pad (out, new_hdr_end - new_rels_end);
  skip (in1, hdr_end - rels_end);
  copy (in1, out, tot_sz - hdr_end, hdr_end, new_hdr_end);
  pad (out, aligned_tot_sz - tot_sz);

  close_out (out);
  fclose (in1);
}

static void
stubify (void)
{
  uint_le32_t ne_magic_le;
  size_t magic_len = sizeof (ne_magic_le);
  ul_t tot1_sz, tot2_sz, new_tot_sz;
  mz_hdr_t mz2;
  uint16_t lfarlc, oem;
  uint32_t mz2_sz, stub_sz, lfanew = 0;
  bool renew = false;

  in1 = open_in (in1_path);
  tot1_sz = size_it (in1);
  process_ne_magic (in1, &ne_magic_le, "stubify");

  in2 = open_in (in2_path);
  process_mz_hdr_common (in2, &mz2, &tot2_sz, &mz2_sz);

  lfarlc = leh16 (mz2.e_lfarlc);
  stub_sz = mz2_sz;
  if (lfarlc == MZ_LFARLC_NEW)
    {
      oem = MZ_LFARLC_NEW - offsetof (mz_hdr_t, e_res);
      if (oem != 0 && fread (&mz2.e_res, oem, 1, in2) == 1)
	{
	  lfanew = leh32 (mz2.e_lfanew);
	  if (lfanew != 0 && lfanew <= tot2_sz)
	    {
	      if (! force_cp_cblp)
		stub_sz = lfanew;
	      else if (tot1_sz > (ul_t) UINT32_MAX - MZ_PARA_SZ + 1
		       || stub_sz > (ul_t) UINT32_MAX - MZ_PARA_SZ + 1
				    - tot1_sz)
		error ("output file will be too large, "
		       "%#lx + %#lx > 4 GiB - %#x",
		       (ul_t) stub_sz, tot1_sz, (ui_t) MZ_PARA_SZ);
	      else
		{
		  new_tot_sz = stub_sz + (uint32_t) tot1_sz;
		  new_tot_sz = (new_tot_sz + MZ_PARA_SZ - 1)
			       & -(ul_t) MZ_PARA_SZ;
		  mz2.e_lfanew = hle32 (new_tot_sz);
		  renew = true;
		}
	    }
	}
    }

  out = open_out (out_path);

  if (renew)
    {
      if (fwrite (&mz2, 1, MZ_LFARLC_NEW, out) != MZ_LFARLC_NEW)
	error ("cannot write MZ header");
      copy (in2, out, stub_sz - MZ_LFARLC_NEW, MZ_LFARLC_NEW, MZ_LFARLC_NEW);
    }
  else
    {
      rewind (in2);
      copy (in2, out, stub_sz, 0, 0);
    }
  fclose (in2);
  in2 = NULL;

  if (fwrite (&ne_magic_le, 1, magic_len, out) != magic_len)
    error ("cannot write payload magic number");

  if (renew)
    {
      copy (in1, out, tot1_sz - magic_len, magic_len, stub_sz + magic_len);
      pad (out, new_tot_sz - stub_sz - tot1_sz);
    }
  else
    copy_ne (in1, out, tot1_sz - magic_len, magic_len, stub_sz + magic_len,
	     ne_magic_le);

  close_out (out);
  fclose (in1);
}

static void
unstubify (void)
{
  ul_t tot_sz;
  mz_hdr_t mz;
  uint16_t lfarlc, oem;
  uint32_t mz_sz, stub_sz, lfanew;
  uint_le32_t ne_magic_le;
  size_t magic_len = sizeof (ne_magic_le);

  in1 = open_in (in1_path);
  process_mz_hdr_common (in1, &mz, &tot_sz, &mz_sz);

  lfarlc = leh16 (mz.e_lfarlc);
  stub_sz = mz_sz;
  if (lfarlc == MZ_LFARLC_NEW)
    {
      oem = MZ_LFARLC_NEW - offsetof (mz_hdr_t, e_res);
      if (oem != 0 && fread (&mz.e_res, oem, 1, in1) == 1)
	{
	  lfanew = leh32 (mz.e_lfanew);
	  if (lfanew != 0 && lfanew <= tot_sz)
	    stub_sz = lfanew;
	}
    }

  if (stub_sz >= tot_sz)
    error ("no non-stub content to unstubify");
  else if (tot_sz - stub_sz < 2)
    error ("too little non-stub content to unstubify?");

  if (fseek (in1, stub_sz, SEEK_SET) != 0)
    error ("cannot seek to end of stub");

  process_ne_magic (in1, &ne_magic_le, "unstubify");

  out = open_out (out_path);
  if (fwrite (&ne_magic_le, 1, magic_len, out) != magic_len)
    error ("cannot write payload magic number");
  copy_ne (in1, out, tot_sz - stub_sz - magic_len,
	   stub_sz + magic_len, (uint32_t) magic_len, ne_magic_le);

  close_out (out);
  fclose (in1);
}

static void
process (void)
{
  switch (op_mode)
    {
    default:
      lfanew ();
      break;

    case MODE_STUBIFY:
      stubify ();
      break;

    case MODE_UNSTUBIFY:
      unstubify ();
    }
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
  process ();
  return 0;
}
