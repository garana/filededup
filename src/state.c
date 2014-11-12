
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

#include "state.h"
#include "config.h"

#include <string.h>

devino_t* devino_init(devino_t* di, dev_t dev, ino_t inode) { // {{{
	di->inode = inode;
	di->dev = dev;
	return di;
} // }}}

cluster_t* cluster_init(cluster_t* c) { // {{{
	htable_init(&c->files, 0);
	return c;
} // }}}

cluster_t* cluster_new() { // {{{
	return cluster_init((cluster_t*)calloc(1, sizeof(cluster_t)));
} // }}}

void* cluster_destroy(cluster_t* c) { // {{{
	htable_destroy(&c->files);
	return c;
} // }}}

void cluster_delete(cluster_t* c) { // {{{
	free(cluster_destroy(c));
} // }}}

file_t* file_new(const char* path, struct stat* st, cluster_t* cluster) { // {{{
	file_t* f = (file_t*)calloc(1, sizeof(file_t));

	f->path = strdup(path);
	memcpy(&f->st, st, sizeof(f->st));
	f->cluster = cluster;

	f->key = NULL;

	return f;
} // }}}

void* file_destroy(file_t* f) { // {{{
	free((void*)f->path);
	return f;
} // }}}

void file_delete(file_t* f) { // {{{
	free(file_destroy(f));
} // }}}

run_state* state() { // {{{
	static run_state s;
	return &s;
} // }}}

run_state* state_init(run_state* r) { // {{{

	r->idiscriminant = 0;
	r->saved = 0;

	htable_init(&r->filesByDevIno, 8);
	htable_init(&r->clustersByKey, 8);

	return r;
} // }}}

void state_setup() { // {{{
	CACHED_CONFIG(cfg);

	run_state* s = state_init(state());

	digest_setup(cfg->discriminantv[s->idiscriminant].methods);
} // }}}

int state_next_step(run_state* old) { // {{{

	CACHED_CONFIG(cfg);

	run_state* s = state();

	memcpy(old, s, sizeof(*s));

	if (++s->idiscriminant == cfg->discriminantc)
		return 0;

	htable_init(&s->filesByDevIno, 8);
	htable_init(&s->clustersByKey, 8);

	digest_setup(cfg->discriminantv[s->idiscriminant].methods);

	return 1;

} // }}}

struct discriminant_t* current_discriminant() { // {{{

	CACHED_CONFIG(cfg);
	run_state* s = state();

	if (s->idiscriminant >= cfg->discriminantc)
		return NULL;

	if (s->idiscriminant < 0)
		return NULL;

	return cfg->discriminantv + s->idiscriminant;

} // }}}

DEFINE_HTABLE_TYPE_KL(clfiles, char, file_t);
DEFINE_HTABLE_TYPE(devino2file, devino_t, file_t);
DEFINE_HTABLE_TYPE_KL(key2cluster, long, cluster_t);

