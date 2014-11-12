
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

#ifndef __FILEDEDUP_DIGEST_H
#define __FILEDEDUP_DIGEST_H

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/evp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct digest_t {
	unsigned char md5[MD5_DIGEST_LENGTH];
	unsigned char sha1[SHA_DIGEST_LENGTH];
	unsigned char sha224[SHA224_DIGEST_LENGTH];
	unsigned char sha256[SHA256_DIGEST_LENGTH];
	unsigned char sha384[SHA384_DIGEST_LENGTH];
	unsigned char sha512[SHA512_DIGEST_LENGTH];
	unsigned char ripemd160[RIPEMD160_DIGEST_LENGTH];
} digest_t;

typedef struct digest_state_t {
	EVP_MD_CTX* md5;
	EVP_MD_CTX* sha1;
	EVP_MD_CTX* sha224;
	EVP_MD_CTX* sha256;
	EVP_MD_CTX* sha384;
	EVP_MD_CTX* sha512;
	EVP_MD_CTX* ripemd160;
} digest_state_t;

void digest_setup();
int digest_init(digest_state_t*);
void digest_update(digest_state_t*, const void* b, size_t len);
void digest_final(digest_state_t*, digest_t*);
void digest_clean();

int digest_file(const char* s, struct stat* _st, struct digest_t* digest);

#endif

