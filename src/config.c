
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

#include "config.h"
#include "error.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int path_source_set(int *cfg_flag, int value) { // {{{
	*cfg_flag &= ~PATHSOURCE_MASK;
	*cfg_flag |= (value & PATHSOURCE_MASK);
	return *cfg_flag;
} // }}}

struct config_t* config() { // {{{
	static struct config_t c;
	return &c;
} // }}}

struct config_t* config_init(struct config_t* cfg) { // {{{
	cfg->flags = PATHSOURCE_ARGS | LINK_TYPE_HARD;
	cfg->verbose = 0;
	cfg->nice = 0;
	cfg->ionice = 0;
	cfg->read_policy = 'm';
	cfg->bufsize = 4096*4096;
	cfg->minage = 0;
	cfg->cgroups = NULL;
	cfg->cgroupc = 0;

	memset(&cfg->discriminantv[0], 0, sizeof(cfg->discriminantv));
	discriminantv_parse("dev,size,perms,user,group", &cfg->discriminantv[0]);
	discriminantv_parse("sha1:4096", &cfg->discriminantv[1]);
	discriminantv_parse("sha512,ripemd160", &cfg->discriminantv[2]);
	cfg->discriminantc = 3;

	cfg->report_file = NULL;
	cfg->report_fd = -1;
	cfg->report_separator = '\0';

	return cfg;
} // }}}

int verbose() { // {{{

	static int _level = -1;

	if (_level < 0)
		_level = config()->verbose;

	return _level;

} // }}}

int dryrun() { // {{{

	static int _dryrun = -1;

	if (_dryrun < 0)
		_dryrun = config()->flags & CONFIG_DRYRUN ? 1 : 0;

	return _dryrun;

} // }}}

