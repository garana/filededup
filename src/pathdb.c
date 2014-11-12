
/*
       This file is part of Filededup, a file deduplication program.
       Copyright (C) 2014 Gonzalo Arana <gonzalo.arana@gmail.com>
       
       Filededup is free software: you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation, either version 3 of the License, or
       (at your option) any later version.
       
       Filededup is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.
       
       You should have received a copy of the GNU General Public License
       along with Filededup.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "pathdb.h"
#include "discriminant.h"

#include <stdlib.h>
#include <string.h>

static const char** pathv = NULL;
static int pathc = 0;
static int pathv_size = 0;

void add_path(const char* path) {
	if (pathv_size == pathc) {
		if (!pathv_size)
			pathv_size = 1;
		pathv = realloc(pathv, sizeof(pathv[0]) * pathv_size * 8);
		pathv_size *= 8;
	}

	pathv[pathc++] = strdup(path);
}

void foreach_path(fe_path_fn fn) {

	int ipath = 0;
	while (ipath < pathc)
		fn(pathv[ipath++]);
}

