
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

#include "htable.h"
#include "memory.h"

#include <string.h>
#include <assert.h>

#define LOAD_FACTOR   8
#define INITIAL_SIZE 16

struct bucket { // {{{
	bucket* next;

	void* key;
	void* data;

	int hval;
	size_t klen;
	size_t dlen;
}; // }}}

bucket* bucket_new(void* key, size_t klen, void* data, size_t dlen, int hval) { // {{{
	bucket* ret = (bucket*)calloc(1, sizeof(bucket));

	ret->key = key;
	ret->klen = klen;

	ret->data = data;
	ret->dlen = dlen;

	ret->hval = hval;

	ret->next = NULL;

	return ret;
} // }}}

void* bucket_delete(bucket* b) { // {{{
	free(b);
	return b;
} // }}}

htable* htable_init(htable* ht, size_t estimated_size) { // {{{

	if (!estimated_size)
		estimated_size = INITIAL_SIZE;

	ht->bucketc = estimated_size / LOAD_FACTOR;
	if (!ht->bucketc)
		ht->bucketc = 8;
	ht->bucketv = (bucket**)calloc(ht->bucketc, sizeof(bucket*));

	ht->entries = 0;
	
	return ht;

} // }}}

htable* htable_new(size_t estimated_size) { // {{{
	return htable_init(calloc(1, sizeof(htable)), estimated_size);
} // }}}

htable* htable_destroy(htable* ht) { // {{{

	size_t i = ht->bucketc;

	while (i--) {
		bucket* head = ht->bucketv[i];
		while (head) {
			bucket* next = head->next;
			bucket_delete(head);
			head = next;
		}
	}

	ht->entries = 0;
	ht->bucketc = 0;
	free(ht->bucketv);

	return ht;

} // }}}

void htable_delete(htable* ht) { // {{{
	free(htable_destroy(ht));
} // }}}

int htable_hash(void* key, size_t keylen) { // {{{

	int ret = 0;
	unsigned* i = (unsigned*)key;

	while (keylen >= sizeof(unsigned)) {
		ret *= 33;
		ret += *i++;
		keylen -= sizeof(unsigned);
	}

	unsigned char* c = (unsigned char*)i;
	while (keylen > 0) {
		ret *= 33;
		ret += *c++;
		keylen--;
	}

	return ret;
} // }}}

int htable_ibucket(htable* htable, void* key, size_t keylen, int* hval) { // {{{
	*hval = htable_hash(key, keylen);
	return *hval % htable->bucketc;
} // }}}

int htable_load_factor(htable* ht) { // {{{
	return ht->entries / ht->bucketc;
} // }}}

void htable_grow(htable* ht) { // {{{
	size_t new_bucketc = ht->bucketc * 8;
	bucket** new_bucketv = (bucket**)calloc(new_bucketc, sizeof(bucket*));

	size_t i = ht->bucketc;

	while (i--) {
		bucket* head = ht->bucketv[i];
		while (head) {
			bucket* next = head->next;

			int ibucket = head->hval % new_bucketc;
			head->next = new_bucketv[ibucket];
			new_bucketv[ibucket] = head;
			
			head = next;
		}
	}

	free(ht->bucketv);
	ht->bucketc = new_bucketc;
	ht->bucketv = new_bucketv;

} // }}}

bucket** htable_find_pbucket(htable* ht, void* key, size_t keylen, int* hval) { // {{{

	int ibucket = htable_ibucket(ht, key, keylen, hval);

	bucket** b = &ht->bucketv[ibucket];

	for (; *b; b = &b[0]->next) {

		if (keylen != b[0]->klen)
			continue;

		if (!memcmp(key, b[0]->key, keylen))
			return b;
	}

	return b;

} // }}}

int htable_add(htable* ht, void* key, size_t keylen, void* data, size_t dlen) { // {{{

	if (htable_load_factor(ht) > LOAD_FACTOR)
		htable_grow(ht);

	int hval;

	bucket** b = htable_find_pbucket(ht, key, keylen, &hval);

	if (*b)
		return HTABLE_NOT_FOUND;

	*b = bucket_new(key, keylen, data, dlen, hval);

	ht->entries++;

	return HTABLE_FOUND;
} // }}}

void htable_set(htable* ht, void* key, size_t keylen, void* data, size_t dlen) { // {{{

	if (htable_load_factor(ht) > LOAD_FACTOR)
		htable_grow(ht);

	int hval;
	bucket** b = htable_find_pbucket(ht, key, keylen, &hval);

	if (*b) {
		free(b[0]->data);
		b[0]->data = xmemdup(data, dlen);
		b[0]->dlen = dlen;
	} else {

		*b = bucket_new(key, keylen, data, dlen, hval);

		ht->entries++;
	}

} // }}}

int htable_unset(htable* ht, void* key, size_t keylen, void** data, size_t *dlen) { // {{{

	int hval;
	bucket** b = htable_find_pbucket(ht, key, keylen, &hval);

	if (!*b)
		return HTABLE_NOT_FOUND;

	bucket* next = b[0]->next;

	*data = b[0]->data;
	*dlen = b[0]->dlen;
	b[0]->data = NULL;

	bucket_delete(*b);

	*b = next;

	ht->entries--;

	return HTABLE_FOUND;
} // }}}

int htable_find(htable* ht, void* key, size_t keylen, void** data, size_t *dlen) { // {{{

	int hval;
	bucket** b = htable_find_pbucket(ht, key, keylen, &hval);

	if (!*b)
		return HTABLE_NOT_FOUND;

	*data = b[0]->data;
	*dlen = b[0]->dlen;

	return HTABLE_FOUND;
} // }}}

void htable_foreach(htable* ht, htforeach htfe, void* cbdata) { // {{{

	size_t i = ht->bucketc;

	while (i--) {

		bucket* b = ht->bucketv[i];

		while (b) {
			if (htfe(b->key, b->klen, b->data, b->dlen, cbdata) < 0)
				return;
			b = b->next;
		}

	}
} // }}}

void htable_bucketc(htable* ht, unsigned* countv, size_t countc) { // {{{

	memset(countv, 0, sizeof(countv[0]) * countc);

	size_t i = ht->bucketc;

	while (i--) {
		
		bucket* b = ht->bucketv[i];
		unsigned cnt = 0;

		while (b) {
			b = b->next;
			++cnt;
		}

		if (cnt >= countc)
			cnt = countc-1;

		countv[cnt]++;
	}

} // }}}


