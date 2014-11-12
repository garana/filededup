
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

#define _GNU_SOURCE

#include "options.h"
#include "config.h"
#include "error.h"
#include "pathdb.h"
#include "ionice.h"
#include "discriminant.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

extern void help();

int parse_int(const char* value, int on_err, int on_range_err) { // {{{

	if (!*value)
		return on_err;

	char* end = NULL;
	long r = strtol(value, &end, 0);

	if (*end)
		return on_err;

	int _r = r;

	if (_r != r)
		return on_range_err;

	return _r;
} // }}}

unsigned long parse_age(const char* value) { // {{{

	long age = 0;
	char unit = 0;

	if (sscanf(value, "%lu%c", &age, &unit) < 1)
		fatal("Invalid minage argument.\n");

	switch (unit) {
		case '\0':
		case 's':
			break;

		case 'm':
			age *= 60;
			break;

		case 'h':
			age *= 3600;
			break;

		case 'd':
			age *= 86400;
			break;

		case 'w':
			age *= 7*86400;
			break;

		case 'M':
			age *= 30*86400;
			break;

		case 'Y':
			age *= 364L*30*86400;
			break;
	}

	return age;

} // }}}

void parse_read(const char* s, char* read_policy, unsigned* bufsize) { // {{{

	char _policy[5];
	long page_size = sysconf(
#if defined(_SC_PAGESIZE)
		_SC_PAGESIZE
#elif defined(PAGE_SIZE)
		PAGE_SIZE
#elif defined(PAGESIZE)
		PAGESIZE
#else
#error "Don't know how to get the page size"
		-1
#endif
	);

	if (sscanf(s, "%4[readmp],%u", &_policy[0], bufsize) < 1)
		fatal("Unknown read policy spec.\n");

	if (!strcmp(_policy, "read"))
		*read_policy = 'r';

	if (!strcmp(_policy, "mmap")) {
		*read_policy = 'm';
		if (*bufsize % page_size) {
			*bufsize = ((*bufsize / page_size) + 1 ) * page_size;
			warning("Adjusting buffer size to %u (require multiple of %u).\n", *bufsize, page_size); 
		}
	}


} // }}}

void parse_options(int argc, char* argv[]) { // {{{

	int c;
	int digit_optind = 0;

	struct config_t* cfg = config_init(config());

	int _did_reset_discc = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"0",               no_argument,       0, '0' },
			{"minage",          required_argument, 0, 'm' },
			{"eval",            required_argument, 0, 'e' },
			{"hard-link",       no_argument,       0, 'H' },
			{"symbolic-link",   no_argument,       0, 'L' },
			{"show-merge-0",    required_argument, 0, 'O' },
			{"show-merge",      required_argument, 0, 'o' },
			{"dry-run",         no_argument,       0, 'n' },
			{"nice",            required_argument, 0, 'N' },
			{"ionice",          required_argument, 0, 'i' },
			{"cgroup",          required_argument, 0, 'c' },
			{"verbose",         no_argument,       0, 'v' },
			{"jobs",            required_argument, 0, 'j' },
			{"read",            required_argument, 0, 'R' },
			{"help",            no_argument,       0, '?' },
			{0,                 0,                 0,  0  }
		};

		c = getopt_long(argc, argv, "0m:e:HLO:o:nN:i:c:t:vj:R:h",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case '0':
				path_source_set(&cfg->flags, PATHSOURCE_STDIN0);
				break;

			case 'm':
				cfg->minage = parse_age(optarg);
				break;

			case 'e':
				if (!_did_reset_discc) {
					_did_reset_discc = 1;
					cfg->discriminantc = 0;
				}
				discriminantv_parse(optarg, cfg->discriminantv + cfg->discriminantc++);
				break;

			case 'H':
				link_type_set_hard(cfg->flags);
				break;

			case 'L':
				link_type_set_symb(cfg->flags);
				break;

			case 'O':
				cfg->report_file = strdup(optarg);
				cfg->report_separator = '\0';
				break;

			case 'o':
				cfg->report_file = strdup(optarg);
				cfg->report_separator = '\n';
				break;

			case 'n':
				cfg->flags |= CONFIG_DRYRUN;
				break;

			case 'N':
				if (sscanf(optarg, "%d", &cfg->nice) != 1)
					fatal("Invalid nice level (expecting an positive integer value).\n");

				if (cfg->nice < 0)
					fatal("Only positive integers are allowed in nice value.\n");

				break;

			case 'i':
				cfg->ionice = ionice_parse(optarg);
				break;

			case 'c':
				cfg->cgroups = realloc(cfg->cgroups, ++cfg->cgroupc * sizeof(char*));
				cfg->cgroups[cfg->cgroupc-1] = strdup(optarg);
				break;

			case 'v':
				cfg->verbose++;
				break;

			case 'R':
				parse_read(optarg, &cfg->read_policy, &cfg->bufsize);
				break;

			case '?':
			case 'h':
				help();
				exit(0);

			default:
				fatal("Unknown switch %s\n.", argv[option_index]);
		}
	}

	discriminantv_post_parse(cfg->discriminantv, cfg->discriminantc);

	if (cfg->report_file) {
		cfg->report_fd = open(cfg->report_file, O_CREAT | O_RDWR | O_TRUNC, 0640);
		if (cfg->report_fd < 0)
			fatal("Could not open file %s: %s.\n", cfg->report_file, strerror(errno));
	}

	if (optind < argc) {
		if ((cfg->flags & PATHSOURCE_ARGS) == 0)
			fatal("Extra arguments found.\n");

		while (optind < argc)
			add_path(argv[optind++]);

	} else if ((cfg->flags & PATHSOURCE_STDIN) == 0) {
		path_source_set(&cfg->flags, PATHSOURCE_STDIN);

	}
} // }}}

