
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

#include <stdio.h>
#include <assert.h>
#include <string.h>

int check(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {
	assert(keylen == sizeof(int));
	assert(!cbdata);
	int i = *(int*)key;
//	long l = *(long*)data;

	long l, a;
	int nfields = sscanf((char*)data, "%ld %ld", &a, &l);

	assert(i == l);
	return 0;
}

void show_counth(htable* ht) {
	unsigned countv[8];
	htable_bucketc(ht, &countv[0], sizeof(countv)/sizeof(countv[0]));

	int i = 0;

	for (; i < sizeof(countv)/sizeof(countv[0]); ++i)
		fprintf(stdout, "[%d] = %u\n", i, countv[i]);
}

int main(int argc, char* argv[]) {

	htable ht;

	htable_init(&ht, 1);

	int i = 0;
	long l = 0;

	while (i < 1000) {
		char* k = NULL;
		asprintf(&k, "%ld %ld", rand(), l);
		htable_add(&ht, xmemdup(&i, sizeof(i)), sizeof(i), k, strlen(k)+1);
		++i;
		++l;

//		if (!(i % 100)) {
//			show_counth(&ht);
//			fprintf(stdout, "\n\n");
//		}
	}

	htable_foreach(&ht, check, NULL);

	htable_destroy(&ht);

}

