/*
 * Copyright (c) 2023 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <lfanew/dirent.h>

#define ERROR		return cleanup (dir, names, n)

static int
cleanup (DIR *dir, struct dirent **names, unsigned n)
{
  int save_errno = errno;
  closedir (dir);
  if (n)
    {
      while (n-- != 0)
	free (names[n]);
      free (names);
    }
  errno = save_errno;
  return -1;
}

int
scandir (const char *path, struct dirent ***p_names,
	 int (*filt) (const struct dirent *),
	 int (*comp) (const struct dirent **, const struct dirent **))
{
  int save_errno = errno;
  struct dirent *ent, *ent2, **names = NULL, **names2;
  unsigned n = 0, top = 0;
  DIR *dir = opendir (path);

  if (! dir)
    return -1;

  while (errno = 0,
	 (ent = readdir (dir)) != NULL)
    {
      if (filt && ! filt (ent))
	continue;

      if (n >= top)
	{
	  if (top > ((unsigned) INT_MAX - 16U) / 2U)
	    {
	      errno = ENOMEM;
	      ERROR;
	    }
	  top = 2U * top + 16U;
	  names2 = realloc (names, top * sizeof (struct dirent *));
	  if (! names2)
	    ERROR;
	  names = names2;
	}

      ent2 = _direntdup (ent);
      if (! ent2)
	ERROR;

      names[n] = ent2;
      ++n;
    }

  if (errno != 0
      || closedir (dir) == -1)
    ERROR;

  if (comp)
    qsort (names, n, sizeof (struct dirent *),
	   (int (*) (const void *, const void *)) comp);

  *p_names = names;
  errno = save_errno;
  return (int) n;
}
