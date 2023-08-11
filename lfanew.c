/*
 * Copyright (c) 2023 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lfanew/io.h>
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
static bool keep_output = false, force_cp_cblp = false, unsafe = false;
static const char *me = NULL, *out_path = NULL,
		  *in1_path = NULL, *in2_path = NULL;
static int in1 = -1, in2 = -1, out = -1;

static void
usage (void)
{
  fprintf
    (stderr, "usage:\n"
	     "  # to add .e_lfanew\n"
	     "  %s [-k] -o OUT-STUB-FILE IN-STUB-FILE\n"
	     "  # to stubify\n"
	     "  %s -S [-kp] -o OUT-FAT-FILE IN-PAYLOAD-FILE IN-STUB-FILE\n"
	     "  # to unstubify\n"
	     "  %s -U [-kp] -o OUT-PAYLOAD-FILE IN-FAT-FILE\n",
     me, me, me);
  exit (1);
}

static void
clean_up (void)
{
  if (out != -1)
    close (out);
  if (! keep_output && out_path)
   remove (out_path);
  if (in1 != -1)
    close (in1);
  if (in2 != -1)
    close (in2);
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
  while ((opt = getopt (argc, argv, "ko:pSU_:")) != -1)
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
	case '_':
	  if (optarg[0] == '-' && optarg[1] == 0)
	    {
	      unsafe = true;
	      break;
	    }
	  /* FALLTHRU */
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

static int
open_in (const char *path)
{
  int in = open (path, O_RDONLY);
  if (in == -1)
    error_with_errno ("cannot open input file");
  if (_binmode (in) == -1)
    error_with_errno ("cannot open input file");
  return in;
}

static int
open_out (const char *path)
{
  int out = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (out == -1)
    error_with_errno ("cannot open output file");
  if (_binmode (out) == -1)
    error_with_errno ("cannot open output file");
  return out;
}

static void
close_out (int out)
{
  if (close (out) == -1)
    error_with_errno ("cannot close output file");
}

static ul_t
size_it (int in)
{
  off_t pos, end_pos;
  ul_t tot_sz;

  pos = lseek (in, (off_t) 0, SEEK_CUR);
  if (pos == (off_t) -1)
    error_with_errno ("cannot get size of input file");

  end_pos = lseek (in, (off_t) 0, SEEK_END);
  if (end_pos == (off_t) -1)
    error_with_errno ("cannot get size of input file");

  if ((uintmax_t) end_pos > ULONG_MAX)
    error ("size of input file exceeds %#lx", (ul_t) ULONG_MAX);

  tot_sz = (ul_t) end_pos;

  if (lseek (in, pos, SEEK_SET) != pos)
    error_with_errno ("cannot get size of input file");

  return tot_sz;
}

static bool
xread1 (int in, void *buf, size_t n)
{
  char *p = (char *) buf;
  ssize_t r;
  while (n != 0)
    {
      errno = 0;
      r = read (in, p, n);
      if (r <= 0)
	{
	  int err = errno;
	  if (err != EAGAIN && err != EINTR)
	    return false;
	}
      else
	{
	  p += (size_t) r;
	  n -= (size_t) r;
	}
    }
  return true;
}

static void
xread (int in, void *buf, size_t n)
{
  if (! xread1 (in, buf, n))
    error_with_errno ("cannot read input file");
}

static bool
xwrite1 (int out, const void *buf, size_t n)
{
  const char *p = (const char *) buf;
  ssize_t r;
  while (n != 0)
    {
      errno = 0;
      r = write (out, p, n);
      if (r <= 0)
	{
	  int err = errno;
	  if (err != EAGAIN && err != EINTR)
	    return false;
	}
      else
	{
	  p += (size_t) r;
	  n -= (size_t) r;
	}
    }
  return true;
}

static void
xwrite (int out, const void *buf, size_t n)
{
  if (! xwrite1 (out, buf, n))
    error_with_errno ("cannot write output file");
}

static void
process_old_magic (int in, uint_le16_t *p_old_magic_le)
{
  size_t magic_len = sizeof (*p_old_magic_le);
  xread (in, p_old_magic_le, magic_len);
}

static void
process_mz_hdr_common (int in, uint_le16_t old_magic_le,
		       mz_hdr_t *pmz, ul_t *p_tot_sz, uint32_t *p_mz_sz)
{
  ul_t tot_sz;
  uint16_t old_magic, lfarlc, cp, cblp, cparhdr;
  uint32_t mz_sz, hdr_end;

  tot_sz = size_it (in);
  if (tot_sz < offsetof (mz_hdr_t, e_res))
    error ("input is not MZ file");

  old_magic = leh16 (old_magic_le);
  if (old_magic != MZ_MAGIC && old_magic != MZ_MAGIC_ALT)
    error ("input is not MZ file");

  memset (pmz, 0, sizeof (mz_hdr_t));
  pmz->e_magic = old_magic_le;
  xread (in, &pmz->e_cblp,
	 offsetof (mz_hdr_t, e_res) - offsetof (mz_hdr_t, e_cblp));

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
	warn ("MZ e_cblp == %#x >= %#x looks bogus",
	      (ui_t) cblp, (ui_t) MZ_PG_SZ);
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
process_new_magic (int in, uint_le32_t *p_new_magic_le, const char *what)
{
  size_t magic_len = sizeof (*p_new_magic_le);
  uint16_t new_magic_lo;

  xread (in, p_new_magic_le, magic_len);

  if (unsafe)
    return;

  new_magic_lo = leh32lo (*p_new_magic_le);
  switch (new_magic_lo)
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
pad (int out, size_t n)
{
  char buf[MZ_PG_SZ];
  memset (buf, 0, sizeof (buf));
  while (n >= sizeof (buf))
    {
      xwrite (out, buf, sizeof (buf));
      n -= sizeof (buf);
    }
  xwrite (out, buf, n);
}

static void
skip (int in, size_t n)
{
  char buf[MZ_PG_SZ];
  while (n >= sizeof (buf))
    {
      xread (in, buf, sizeof (buf));
      n -= sizeof (buf);
    }
  xread (in, buf, n);
}

static void
copy (int in, int out, ul_t sz, uint32_t in_off, uint32_t out_off)
{
  char buf[MZ_PG_SZ];
  while (sz)
    {
      size_t n = sz < sizeof (buf) ? sz : sizeof (buf);
      if (! xread1 (in, buf, n))
	error_with_errno ("cannot read input file at offset %#lx",
			  (ul_t) in_off);
      in_off += n;
      if (! xwrite1 (out, buf, n))
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

static ul_t
round_up_to_two_power (ul_t value, ul_t multiple)
{
  return (value + multiple - 1) & -multiple;
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
copy_pe (int in, int out, ul_t sz, uint32_t in_off, uint32_t out_off)
{
  uint32_t inh_size, aligned_inh_size, outh_size, aligned_outh_size,
	   inh_slack, outh_slack, file_align, sect_align, adjust;
  pe_img_fil_hdr_t fhdr;
  pe_img_opt_hdr_t ophdr;
  uint16_t ophdr_size, ophdr_magic, num_sects;
  uint32_t min_used_rva = UINT32_MAX, rva;
  unsigned i;
  pe_sect_hdr_t sect;
  bool pe32plus = false;

  if (sizeof (fhdr) > sz)
    error ("PE COFF file header overshoots file end");

  if (! xread1 (in, &fhdr, sizeof (fhdr)))
    error_with_errno ("cannot read PE COFF file header");

  if (leh32 (fhdr.PointerToSymbolTable) != 0
      || leh32 (fhdr.NumberOfSymbols) != 0)
    error ("PE COFF symbol table unsupported");

  if (! xwrite1 (out, &fhdr, sizeof (fhdr)))
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
  if (! xread1 (in, &ophdr, ophdr_size))
    error_with_errno ("cannot read PE optional header");

  file_align = leh32 (ophdr.h.FileAlignment);
  if (! is_two_power (file_align))
    error ("PE FileAlignment %#lx not power of 2", (ul_t) file_align);
  sect_align = leh32 (ophdr.h.SectionAlignment);
  if (! is_two_power (sect_align))
    error ("PE SectionAlignment %#lx not power of 2", (ul_t) sect_align);
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

  aligned_outh_size = round_up_to_two_power (outh_size, file_align);
  if (aligned_outh_size < outh_size)
    error ("output PE headers will be too large, %#lx + %#lx > 4 GiB",
	   (ul_t) outh_size, (ul_t) file_align);
  ophdr.h.SizeOfHeaders = hle32 (aligned_outh_size);
  adjust = aligned_outh_size - aligned_inh_size;

  ophdr_magic = leh16 (ophdr.h.Magic);
  switch (ophdr_magic)
    {
    default:
      error ("PE optional header type %#x unsupported", (ui_t) ophdr_magic);

    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:  /* PE32 */
      if (ophdr_size > sizeof (ophdr.h32))
	error ("long PE32 header unsupported, %#x > %#x",
	       (ui_t) ophdr_size, (ui_t) sizeof (ophdr.h32));

      for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
	{
	  pe_data_dir_t *ent = &ophdr.h32.DataDirectory[i];
	  switch (i)
	    {
	    case IMAGE_DIRECTORY_ENTRY_SECURITY:
	      pe_adjust_off (&ent->VirtualAddress, aligned_inh_size, adjust);
	      break;

	    case IMAGE_DIRECTORY_ENTRY_DEBUG:
	      /* FIXME */
	      if (leh32 (ent->VirtualAddress) != 0 && leh32 (ent->Size) != 0)
		warn ("PE32 debug table unsupported; retaining anyway");
	      /* FALLTHRU */

	    default:
	      rva = leh32 (ent->VirtualAddress);
	      if (rva != 0 && min_used_rva > rva)
		min_used_rva = rva;
	    }
	}
      break;

    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:  /* PE32+ */
      pe32plus = true;

      for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
	{
	  pe_data_dir_t *ent = &ophdr.h64.DataDirectory[i];
	  switch (i)
	    {
	    case IMAGE_DIRECTORY_ENTRY_SECURITY:
	      pe_adjust_off (&ent->VirtualAddress, aligned_inh_size, adjust);
	      break;

	    case IMAGE_DIRECTORY_ENTRY_DEBUG:
	      /* FIXME */
	      if (leh32 (ent->VirtualAddress) != 0 && leh32 (ent->Size) != 0)
		warn ("PE32+ debug table unsupported; retaining anyway");
	      /* FALLTHRU */

	    default:
	      rva = leh32 (ent->VirtualAddress);
	      if (rva != 0 && rva < min_used_rva)
		min_used_rva = rva;
	    }
	}
    }

  if (! xwrite1 (out, &ophdr, ophdr_size))
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

      if (! xread1 (in, &sect, sizeof (sect)))
	error_with_errno ("cannot read PE section header #%#x", (ui_t) i);

      rva = leh32 (sect.VirtualAddress);
      if (rva == 0)
	warn ("section #%#x has suspicious RVA of 0", (ui_t) i);
      else if (i == 1
	       && leh32 (sect.SizeOfRawData) == 0
	       && (leh32 (sect.Characteristics)
		   & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA
		      | IMAGE_SCN_CNT_UNINITIALIZED_DATA))
		  == IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	{
	  /*
	   * The first section is a "slack space" section which only contains
	   * uninitialized data.  Adjust its starting RVA and size so that
	   * it just fills the "gap" between the end of the headers & the
	   * section itself.
	   */
	  uint32_t sect_aligned_outh_size
	    = round_up_to_two_power (outh_size, sect_align);
	  uint32_t end_rva = rva + leh32 (sect.VirtualSize);
	  if (! sect_aligned_outh_size)
	    error ("not enough RVA space for slack section, PE headers "
		   "are almost 4 GiB");
	  if (sect_aligned_outh_size > end_rva)
	    error ("not enough RVA space for slack section, %#lx < %#lx",
		   (ul_t) end_rva, (ul_t) sect_aligned_outh_size);
	  rva = sect_aligned_outh_size;
	  sect.VirtualAddress = hle32 (rva);
	  sect.VirtualSize = hle32 (end_rva - rva);
	}
      else if (rva < min_used_rva)
	min_used_rva = rva;

      pe_adjust_off (&sect.PointerToRawData, aligned_inh_size, adjust);
      pe_adjust_off (&sect.PointerToRelocations, aligned_inh_size, adjust);
      pe_adjust_off (&sect.PointerToLinenumbers, aligned_inh_size, adjust);

      if (! xwrite1 (out, &sect, sizeof (sect)))
	error_with_errno ("cannot write PE section header #%#x", (ui_t) i);

      in_off += sizeof (sect);
      out_off += sizeof (sect);
      sz -= sizeof (sect);
    }

  /*
   * We need enough virtual address space between {ImageBase} & {ImageBase
   * + minimum used RVA} to accommodate all the headers, i.e. MZ stub, PE
   * headers, & section headers.  For now, simply error out if the minimum
   * used RVA is too small.  (Alternatively, we could try to adjust all the
   * RVAs in the program upwards, but this is hard to do right & not a lot
   * of fun.)
   */
  if (! unsafe && min_used_rva < aligned_outh_size)
    error ("not enough RVA space for output PE headers, %#lx < %#lx",
	   (ul_t) min_used_rva, (ul_t) aligned_outh_size);

  rva = leh32 (ophdr.h.BaseOfCode);
  if (rva < min_used_rva)
    warn ("PE BaseOfCode == %#lx < %#lx looks bogus",
	  (ul_t) rva, (ul_t) min_used_rva);
  if (! pe32plus)
    {
      rva = leh32 (ophdr.h32.BaseOfData);
      if (rva < min_used_rva)
	warn ("PE32 BaseOfData == %#lx < %#lx looks bogus",
	      (ul_t) rva, (ul_t) min_used_rva);
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
copy_new (int in, int out, ul_t sz, uint32_t in_off, uint32_t out_off,
	  uint_le32_t new_magic_le)
{
  switch (leh32 (new_magic_le))
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
  uint_le16_t old_magic_le;
  mz_hdr_t mz;
  uint16_t lfarlc, oem, cparhdr, crlc;
  uint32_t mz_sz, new_mz_sz, new_tot_sz, rels_sz, rels_end, hdr_end,
	   new_rels_end, new_hdr_end;

  in1 = open_in (in1_path);
  process_old_magic (in1, &old_magic_le);
  process_mz_hdr_common (in1, old_magic_le, &mz, &tot_sz, &mz_sz);

  if (tot_sz > (ul_t) UINT32_MAX - MZ_PARA_SZ + 1)
    error ("input file too large, %#lx > 4 GiB - %#x",
	   tot_sz, (ui_t) MZ_PARA_SZ);
  aligned_tot_sz = round_up_to_two_power (tot_sz, MZ_PARA_SZ);

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
  if (oem != 0 && ! xread1 (in1, mz.e_res, oem))
    error ("cannot read optional header information");

  new_rels_end = MZ_LFARLC_NEW + (uint32_t) crlc * MZ_RELOC_SZ;
  new_hdr_end = round_up_to_two_power (new_rels_end, MZ_PARA_SZ);
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

  if (mz_sz > (uint32_t) 0xffff * MZ_PG_SZ)
    error ("output MZ stub will be too large, %#lx > %#lx",
	   (ul_t) mz_sz, (ul_t) 0xffff * MZ_PG_SZ);

  mz.e_cblp = hle16 (new_mz_sz % MZ_PG_SZ);
  mz.e_cp = hle16 ((new_mz_sz + MZ_PG_SZ - 1) / MZ_PG_SZ);
  mz.e_cparhdr = hle16 (new_hdr_end / MZ_PARA_SZ);
  mz.e_lfarlc = hle16 (MZ_LFARLC_NEW);
  mz.e_lfanew = hle32 (new_tot_sz);

  out = open_out (out_path);
  xwrite (out, &mz, sizeof (mz));

  copy (in1, out, rels_sz, lfarlc, MZ_LFARLC_NEW);
  pad (out, new_hdr_end - new_rels_end);
  skip (in1, hdr_end - rels_end);
  copy (in1, out, tot_sz - hdr_end, hdr_end, new_hdr_end);
  pad (out, aligned_tot_sz - tot_sz);

  close_out (out);
  close (in1);
}

static void
stubify (void)
{
  uint_le32_t new_magic_le;
  size_t magic_len = sizeof (new_magic_le);
  ul_t tot1_sz, tot2_sz;
  uint_le16_t old_magic_le;
  mz_hdr_t mz2;
  uint16_t lfarlc, oem;
  uint32_t mz2_sz, stub_sz, lfanew = 0;

  in1 = open_in (in1_path);
  tot1_sz = size_it (in1);
  process_new_magic (in1, &new_magic_le, "stubify");

  in2 = open_in (in2_path);
  process_old_magic (in2, &old_magic_le);
  process_mz_hdr_common (in2, old_magic_le, &mz2, &tot2_sz, &mz2_sz);

  lfarlc = leh16 (mz2.e_lfarlc);
  stub_sz = mz2_sz;
  if (lfarlc == MZ_LFARLC_NEW)
    {
      oem = MZ_LFARLC_NEW - offsetof (mz_hdr_t, e_res);
      if (oem != 0)
	{
	  xread (in2, &mz2.e_res, oem);
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
	    }
	}
    }

  out = open_out (out_path);

  errno = 0;
  if (lseek (in2, (off_t) 0, SEEK_SET) != (off_t) 0)
    error_with_errno ("cannot rewind stub file");
  copy (in2, out, stub_sz, 0, 0);
  close (in2);
  in2 = -1;

  if (! xwrite1 (out, &new_magic_le, magic_len))
    error ("cannot write payload magic number");

  copy_new (in1, out, tot1_sz - magic_len, magic_len, stub_sz + magic_len,
	    new_magic_le);

  close_out (out);
  close (in1);
}

static void
unstubify (void)
{
  ul_t tot_sz;
  mz_hdr_t mz;
  uint16_t lfarlc, oem;
  uint32_t mz_sz, stub_sz, lfanew;
  uint_le32_t new_magic_le;
  size_t magic_len = sizeof (new_magic_le);
  uint_le16_t old_magic_le;

  in1 = open_in (in1_path);
  process_old_magic (in1, &old_magic_le);
  process_mz_hdr_common (in1, old_magic_le, &mz, &tot_sz, &mz_sz);

  lfarlc = leh16 (mz.e_lfarlc);
  stub_sz = mz_sz;
  if (lfarlc == MZ_LFARLC_NEW)
    {
      oem = MZ_LFARLC_NEW - offsetof (mz_hdr_t, e_res);
      if (oem != 0)
	{
	  xread (in1, &mz.e_res, oem);
	  lfanew = leh32 (mz.e_lfanew);
	  if (lfanew != 0 && lfanew <= tot_sz && ! force_cp_cblp)
	    stub_sz = lfanew;
	}
    }

  if (stub_sz >= tot_sz)
    error ("no non-stub content to unstubify");
  else if (tot_sz - stub_sz < 2)
    error ("too little non-stub content to unstubify?");

  errno = 0;
  if (lseek (in1, (off_t) stub_sz, SEEK_SET) != (off_t) stub_sz)
    error_with_errno ("cannot seek to end of stub");

  process_new_magic (in1, &new_magic_le, "unstubify");

  out = open_out (out_path);
  if (! xwrite1 (out, &new_magic_le, magic_len))
    error ("cannot write payload magic number");
  copy_new (in1, out, tot_sz - stub_sz - magic_len,
	    stub_sz + magic_len, (uint32_t) magic_len, new_magic_le);

  close_out (out);
  close (in1);
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
