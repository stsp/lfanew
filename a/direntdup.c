/*
 * Copyright (c) 2023--2024 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef __MINGW32__
# define _POSIX_
#endif
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <lfanew/dirent.h>

#define DIRENT_D_NAME_OFF	offsetof (struct dirent, d_name)
#define DIRENT_SIZE		sizeof (struct dirent)
#if __STDC_VERSION__ - 0 >= 201112L
# define DIRENT_ALIGN		_Alignof (struct dirent)
#else
# define DIRENT_ALIGN		1
#endif
#define ROUND_UP(x, y)		(((x) + (y) - 1) / (y) * (y))

struct dirent *
_direntdup (const struct dirent *ent)
{
  struct dirent *ent2;
  size_t reclen;

  if (ROUND_UP (DIRENT_D_NAME_OFF + NAME_MAX, DIRENT_ALIGN) < DIRENT_SIZE)
    reclen = DIRENT_SIZE;
  else
    {
#ifdef _LFANEW_INTERNAL_HAVE_STRUCT_DIRENT_D_NAMLEN
      reclen = DIRENT_D_NAME_OFF + ent->d_namlen + 1;
#else
      reclen = DIRENT_D_NAME_OFF + strlen (ent->d_name) + 1;
#endif
#ifdef _LFANEW_INTERNAL_HAVE_STRUCT_DIRENT_D_RECLEN
      /*
       * Under MinGW, <dirent.h> says that .d_reclen is "always zero".  Thus
       * we cannot rely on its value blindly.  Argh.
       */
      if (ent->d_reclen > reclen)
	reclen = ent->d_reclen;
#endif
    }

  ent2 = malloc (reclen);
  if (ent2)
    memcpy (ent2, ent, reclen);
  return ent2;
}
