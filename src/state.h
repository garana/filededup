
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

#ifndef __FILEDEDUP_STATE_H
#define __FILEDEDUP_STATE_H

#include "discriminant.h"
#include "htable.h"

#include <stdint.h>

typedef struct devino_t devino_t;
typedef struct cluster_t cluster_t;
typedef struct file_t file_t;

struct devino_t {
	ino_t inode;
	dev_t dev;
};

devino_t* devino_init(devino_t* di, dev_t dev, ino_t inode);

struct cluster_t {
	htable files; // key=path (char*), vale=file_t*
};

cluster_t* cluster_init(cluster_t* c);
cluster_t* cluster_new();
void cluster_delete(cluster_t*);

/* clfiles: cluster files */
DECLARE_HTABLE_TYPE_KL(clfiles, char, file_t);

struct file_t {
	const char* path;
	struct stat st;
	cluster_t* cluster;
	long* key;
};

file_t* file_new(const char* path, struct stat* st, cluster_t* cluster);
void* file_destroy(file_t* f);
void file_delete(file_t* f);

typedef struct run_state {
	int idiscriminant;        /** current discriminant index */
	uint64_t saved;           /** space saved */

	htable filesByDevIno;     /** files[dev,ino] */
	htable clustersByKey;     /** cluster[key]   */
} run_state;

DECLARE_HTABLE_TYPE(devino2file, devino_t, file_t);
DECLARE_HTABLE_TYPE_KL(key2cluster, long, cluster_t);

run_state* state();
void state_setup();
int state_next_step();

#define CACHED_STATE(st)                             \
	static struct run_state* st = NULL;          \
	if (!st)                                     \
		st = state();

struct discriminant_t* current_discriminant();

#endif

