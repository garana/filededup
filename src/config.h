
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

#ifndef __FILEDEDUP_CONFIG_H
#define __FILEDEDUP_CONFIG_H

#ifndef MAX_JOBS
#define MAX_JOBS 64
#endif

#define CONFIG_DRYRUN     0x01

/*****************************************************
 *
 * Path Sources
 *   - stdin
 *   - stdin (NUL terminated)
 *   - args
 *
 */
#define PATHSOURCE_NONE    0x00
#define PATHSOURCE_STDIN   0x02
#define PATHSOURCE_STDIN0 (0x04 | PATHSOURCE_STDIN)
#define PATHSOURCE_ARGS    0x08
#define PATHSOURCE_MASK    (PATHSOURCE_STDIN0 | PATHSOURCE_ARGS )
#define path_source_is_stdin(m) ((m) & PATHSOURCE_STDIN)
#define path_source_is_stdin0(m) (((m) & PATHSOURCE_STDIN0) == PATHSOURCE_STDIN0)
int path_source_set(int *cfg_flag, int value);

/*****************************************************
 *
 * Link Types
 *
 */
#define LINK_TYPE_HARD     0x10
#define LINK_TYPE_SYMB     0x00
#define LINK_TYPE_MASK     0x10
#define link_type_is_hard(m) ((m) & LINK_TYPE_HARD)
#define link_type_is_symb(m) (!link_type_is_hard(m))
#define link_type_set_hard(m)   ((m) |= LINK_TYPE_HARD)
#define link_type_set_symb(m)   ((m) &= ~LINK_TYPE_HARD)
int link_type_aton(const char* s);
char* link_type_ntoa(int m);

#include "discriminant.h"

struct config_t {

	int flags; /* CONFIG_DRYRUN | PATHSOURCE_* | LINK_TYPE_* */
	int verbose;

	int nice;
	int ionice;
	char read_policy; /* 'r': read, 'm': mmap */
	unsigned bufsize;
	unsigned long minage;
	char** cgroups;
	int cgroupc;

	struct discriminant_t discriminantv[DISC_METHODC];
	int discriminantc;

	char* report_file;
	char report_separator;
	int report_fd;
};

struct config_t* config();
struct config_t* config_init(struct config_t* cfg);

#define CACHED_CONFIG(cfg)                           \
	static struct config_t* cfg = NULL;          \
	if (!cfg)                                    \
		cfg = config();

int verbose();
int dryrun();

#endif

