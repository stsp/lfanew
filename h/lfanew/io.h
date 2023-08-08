/*
 * Copyright (c) 2023 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LFANEW_LFANEW_IO_H_
#define _LFANEW_LFANEW_IO_H_

#include <lfanew/_config.h>
#include <fcntl.h>
#ifdef __unix__
# include <unistd.h>
#else
# include <io.h>
#endif

static int
_binmode (int __fd)
{
#if defined _LFANEW_INTERNAL_HAVE__SETMODE \
    && defined _LFANEW_INTERNAL_HAVE_O_BINARY
  return _setmode (__fd, O_BINARY);
#elif defined _LFANEW_INTERNAL_HAVE__SETMODE \
      && defined _LFANEW_INTERNAL_HAVE__O_BINARY
  return _setmode (__fd, _O_BINARY);
#else
  return 0;
#endif
}

#endif
