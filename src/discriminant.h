
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

#ifndef __FILEDEDUP_DISCRIMINANT_H
#define __FILEDEDUP_DISCRIMINANT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>

/*****************************************************
 *
 * Discriminants
 *
 */

#define DISC_NONE           0x0000
                            
#define DISC_DEV            0x0001
#define DISC_SIZE           0x0002
#define DISC_MTIME          0x0004
#define DISC_USER           0x0008
#define DISC_GROUP          0x0010
#define DISC_PERMS          0x0020
#define DISC_STAT_MASK      0x003f
#define DISC_BASENAME       0x0040
                            
#define DISC_MD5            0x0200
#define DISC_SHA1           0x0400
#define DISC_SHA224         0x0800
#define DISC_SHA256         0x1000
#define DISC_SHA384         0x2000
#define DISC_SHA512         0x4000
#define DISC_RIPEMD160      0x8000
#define DISC_CONTENT_MASK   0xff00
                              
#define DISC_METHODC            14

struct discriminant_t {
	int methods;

	/* This is used only when building hashes of parts of the content */
	unsigned long long end;
};

struct discriminant_t* disc_init(struct discriminant_t* t);
void discriminantv_parse(const char* s, struct discriminant_t* disc);
void discriminant_parse(const char* s, struct discriminant_t* disc);
void discriminantv_post_parse(struct discriminant_t* discv, size_t discc);

#include "digest.h"
size_t key_size(struct discriminant_t* d);
void* key_alloc(struct discriminant_t* d, size_t* size);
void* key_new(struct discriminant_t* d, struct stat* st, const char* filename, digest_t* digest, size_t *size);
void key_delete(void* key);

#endif

