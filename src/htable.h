
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

#ifndef __FILEDEDUP_HTABLE_H
#define __FILEDEDUP_HTABLE_H

#include <stdlib.h>

#define HTABLE_FOUND      0
#define HTABLE_NOT_FOUND -1

typedef struct bucket bucket;

typedef struct htable {
	bucket** bucketv; /** array of buckets  */
	size_t bucketc;   /** number of buckets */
	size_t entries;   /** number of entries */
} htable;

htable* htable_init(htable* ht, size_t estimated_size); /** alloc && init a htable  */
htable* htable_new(size_t estimated_size);              /** alloc && init a htable  */
htable* htable_destroy(htable*);                        /** destroy an htable       */
void htable_delete(htable*);                            /** destroy and free htable */

typedef int (*htforeach)(void* key, size_t keylen, void* data, size_t dlen, void* cbdata);

int htable_add(htable* ht, void* key, size_t keylen, void* data, size_t dlen);  /** returns -1 if entry already exists */
void htable_set(htable* ht, void* key, size_t keylen, void* data, size_t dlen); /** adds or replace an entry           */
int htable_unset(htable* ht, void* key, size_t keylen, void** data, size_t *dlen); /** removes an entry, returning its value in *data and its size in *dlen */
int htable_find(htable* htable, void* key, size_t keylen, void** data, size_t *dlen);
void htable_foreach(htable* ht, htforeach htfe, void* cbdata);  /** stops when htfe returns < 0 */

void htable_bucketc(htable* ht, unsigned* countv, size_t countc);

#include <assert.h>

/* fixed length keys */
#define DECLARE_HTABLE_TYPE(short_name, key_type, value_type)          /* {{{ */         \
typedef int (*htforeach ## short_name)(key_type* key, value_type* value, void* cbdata);  \
int short_name##_add(htable* ht, key_type* key, value_type* data);                       \
void short_name##_set(htable* ht, key_type* key, value_type* data);                      \
int short_name##_unset(htable* ht, key_type* key, value_type** data);                    \
int short_name##_find(htable* htable, key_type* key, value_type** data); /* }}} */

#define DEFINE_HTABLE_TYPE(short_name, key_type, value_type) /* {{{ */                   \
                                                                                         \
int short_name##_add(htable* ht, key_type* key, value_type* data) {                      \
	return htable_add(ht, key, sizeof(*key), data, sizeof(*data));                   \
}                                                                                        \
                                                                                         \
void short_name##_set(htable* ht, key_type* key, value_type* data) {                     \
	htable_set(ht, key, sizeof(*key), data, sizeof(*data));                          \
}                                                                                        \
                                                                                         \
int short_name##_unset(htable* ht, key_type* key, value_type** data) {                   \
        size_t size = 0;                                                                 \
	int ret = htable_unset(ht, key, sizeof(*key), (void**)data, &size);              \
	if (ret == HTABLE_FOUND)                                                         \
		assert(size == sizeof(value_type));                                      \
	return ret;                                                                      \
}                                                                                        \
                                                                                         \
int short_name##_find(htable* ht, key_type* key, value_type** data) {                    \
        size_t size = 0;                                                                 \
	int ret = htable_find(ht, (void**)key, sizeof(*key), (void**)data, &size);       \
	if (ret == HTABLE_FOUND)                                                         \
		assert(size == sizeof(value_type));                                      \
	return ret;                                                                      \
} /* }}} */


/* KL = key length, variable length keys */

#define DECLARE_HTABLE_TYPE_KL(short_name, key_type, value_type)                          /* {{{ */     \
typedef int (*htforeach ## short_name)(key_type* key, size_t keylen, value_type* value, void* cbdata);  \
int short_name##_add(htable* ht, key_type* key, size_t keylen, value_type* data);                       \
void short_name##_set(htable* ht, key_type* key, size_t keylen, value_type* data);                      \
int short_name##_unset(htable* ht, key_type* key, size_t keylen, value_type** data);                    \
int short_name##_find(htable* htable, key_type* key, size_t keylen, value_type** data); /* }}} */

#define DEFINE_HTABLE_TYPE_KL(short_name, key_type, value_type) /* {{{ */                \
                                                                                         \
int short_name##_add(htable* ht, key_type* key, size_t keylen, value_type* data) {       \
	return htable_add(ht, key, keylen, data, sizeof(*data));                         \
}                                                                                        \
                                                                                         \
void short_name##_set(htable* ht, key_type* key, size_t keylen, value_type* data) {      \
	htable_set(ht, key, keylen, data, sizeof(*data));                                \
}                                                                                        \
                                                                                         \
int short_name##_unset(htable* ht, key_type* key, size_t keylen, value_type** data) {    \
        size_t size = 0;                                                                 \
	int ret = htable_unset(ht, key, keylen, (void**)data, &size);                    \
	if (ret == HTABLE_FOUND)                                                         \
		assert(size == sizeof(value_type));                                      \
	return ret;                                                                      \
}                                                                                        \
                                                                                         \
int short_name##_find(htable* ht, key_type* key, size_t keylen, value_type** data) {     \
        size_t size = 0;                                                                 \
	int ret = htable_find(ht, (void**)key, keylen, (void**)data, &size);             \
	if (ret == HTABLE_FOUND)                                                         \
		assert(size == sizeof(value_type));                                      \
	return ret;                                                                      \
} /* }}} */

#endif

