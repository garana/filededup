
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
#include "ionice.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

enum {
	IOPRIO_CLASS_NONE,
	IOPRIO_CLASS_RT,
	IOPRIO_CLASS_BE,
	IOPRIO_CLASS_IDLE,
};

enum {
	IOPRIO_WHO_PROCESS = 1,
	IOPRIO_WHO_PGRP,
	IOPRIO_WHO_USER,
};

#define IOPRIO_CLASS_SHIFT      (13)
#define IOPRIO_PRIO_MASK        ((1UL << IOPRIO_CLASS_SHIFT) - 1)

#define IOPRIO_PRIO_CLASS(mask) ((mask) >> IOPRIO_CLASS_SHIFT)
#define IOPRIO_PRIO_DATA(mask)  ((mask) & IOPRIO_PRIO_MASK)
#define IOPRIO_PRIO_VALUE(class, data)  (((class) << IOPRIO_CLASS_SHIFT) | data)

int ionice_parse(const char* value) {

	if (!strcmp(value, "none"))
		return IOPRIO_CLASS_NONE;

	if (!strcmp(value, "idle"))
		return IOPRIO_PRIO_VALUE(IOPRIO_CLASS_IDLE, 7);

	char _class[13];
	int _data = 4;

	if (sscanf(value, "%12[^,],%d", &_class[0], &_data) < 1)
		fatal("Invalid ionice spec.\n");

	if ((_data < 0) || (_data > 7))
		fatal("Invalid ionice data value (must be between 0 and 7).\n");

	if (!strcmp(_class, "realtime") || !strcmp(_class, "rt"))
		return IOPRIO_PRIO_VALUE(IOPRIO_CLASS_RT, _data);

	if (!strcmp(_class, "best-effort") || !strcmp(_class, "be"))
		return IOPRIO_PRIO_VALUE(IOPRIO_CLASS_BE, _data);

	fatal("Invalid ionice spec.\n");
	return -1; /* avoid compiler warning */
}

#include <sys/syscall.h>

static inline int ioprio_set(int which, int who, int ioprio) {
#ifdef SYS_ioprio_set
	return syscall(SYS_ioprio_set, which, who, ioprio);
#else
	return -1;
#endif
}

void ionice_setup() {

	struct config_t* cfg = config();

	if (cfg->ionice > 0)
		ioprio_set(IOPRIO_WHO_PROCESS, 0 /*getpid()*/, cfg->ionice);

}

