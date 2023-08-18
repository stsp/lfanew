/*
 * Copyright (c) 2023 TK Chia
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _LFANEW_LFANEW_DIRENT_H_
#define _LFANEW_LFANEW_DIRENT_H_

#include <lfanew/_config.h>
#include <dirent.h>

#ifndef _LFANEW_INTERNAL_HAVE_SCANDIR
extern int scandir (const char *, struct dirent ***,
		    int (*) (const struct dirent *),
		    int (*) (const struct dirent **, const struct dirent **));
extern int alphasort (const struct dirent **, const struct dirent **);
#endif
extern struct dirent *_direntdup (const struct dirent *);

#endif
