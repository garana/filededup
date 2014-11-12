
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

#include "discriminant.h"
#include "digest.h"
#include "config.h"
#include "string.h"
#include "error.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct discriminant_t* disc_init(struct discriminant_t* t) { // {{{
	memset(t, 0, sizeof(*t));
	return t;
} // }}}

void discriminant_parse(const char* _disc, struct discriminant_t* disc) { // {{{
	if (!strcmp(_disc, "dev"))
		disc->methods |= DISC_DEV;

	else if (!strcmp(_disc, "size"))
		disc->methods |= DISC_SIZE;

	else if (!strcmp(_disc, "mtime"))
		disc->methods |= DISC_MTIME;

	else if (!strcmp(_disc, "user"))
		disc->methods |= DISC_USER;

	else if (!strcmp(_disc, "group"))
		disc->methods |= DISC_GROUP;

	else if (!strcmp(_disc, "perms"))
		disc->methods |= DISC_PERMS;

	else if (!strcmp(_disc, "basename"))
		disc->methods |= DISC_BASENAME;

	else {
		char mech[32];
		unsigned long long end = 0;
		switch (sscanf(_disc, "%31[a-z0-9]:%llu", &mech[0], &end)) {
			case 2:
				break;
			case 1:
				end = 0;
				break;
			default:
				fatal("Invalid discriminant format \"%s\".\n", _disc);
		}

		int idisc = 0;

		mech[31] = '\0';
		if (!strcmp(mech, "md5"))
			idisc = DISC_MD5;

		else if (!strcmp(mech, "sha1"))
			idisc = DISC_SHA1;

		else if (!strcmp(mech, "sha224"))
			idisc = DISC_SHA224;

		else if (!strcmp(mech, "sha256"))
			idisc = DISC_SHA256;

		else if (!strcmp(mech, "sha384"))
			idisc = DISC_SHA384;

		else if (!strcmp(mech, "sha512"))
			idisc = DISC_SHA512;

		else if (!strcmp(mech, "ripemd160"))
			idisc = DISC_RIPEMD160;

		else
			fatal("Unknown discriminant method \"%s\"", _disc);

		if (disc->end)
			fatal("Only one range can be specified per step.");

		disc->methods |= idisc;
		disc->end = end;

	}

} // }}}

void discriminantv_parse(const char* s, struct discriminant_t* disc) { // {{{
	char* _t = strdup(s);
	char* _disc = NULL;

	disc_init(disc);

	while ((_disc = strtok(_disc ? NULL : _t, ",")))
		discriminant_parse(_disc, disc);

	free(_t);

	// shift all the stat pure to the first disc entry

} // }}}

void discriminantv_post_parse(struct discriminant_t* discv, size_t discc) { // {{{

	int i = 1;
	int _disc_methods = 0;

	for (; i < discc; ++i) {
		_disc_methods |= discv[i].methods & DISC_STAT_MASK;
		discv[i].methods &= ~DISC_STAT_MASK;
	}

	if (_disc_methods) {
		warning("Note: size, mtime, user, group, and perms should be used only in the first step.");
		discv[0].methods |= _disc_methods;
	}

	if (link_type_is_hard(config()->flags) && !(discv[0].methods & DISC_DEV)) {
		warning("Note: forcing \"dev\" in step 0 (will merge with hardlinks).\n");
		discv[0].methods |= DISC_DEV;
	}

} // }}}

size_t key_size(struct discriminant_t* d) { // {{{
	size_t ret = sizeof(unsigned long);

	if (d->methods & DISC_DEV)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_SIZE)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_MTIME)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_USER)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_GROUP)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_PERMS)
		ret += sizeof(unsigned long);

	if (d->methods & DISC_BASENAME)
		ret += 2 * sizeof(unsigned long);

	if (d->methods & DISC_MD5)
		ret += MD5_DIGEST_LENGTH;

	if (d->methods & DISC_SHA1)
		ret += SHA_DIGEST_LENGTH;

	if (d->methods & DISC_SHA224)
		ret += SHA224_DIGEST_LENGTH;

	if (d->methods & DISC_SHA256)
		ret += SHA256_DIGEST_LENGTH;

	if (d->methods & DISC_SHA384)
		ret += SHA384_DIGEST_LENGTH;

	if (d->methods & DISC_SHA512)
		ret += SHA512_DIGEST_LENGTH;

	if (d->methods & DISC_RIPEMD160)
		ret += RIPEMD160_DIGEST_LENGTH;

	return ret;
} // }}}

void basename_discriminant(const char* filename, long* h, long* len) { // {{{
	const char* stt = strrchr(filename, '/');
	if (!stt)
		stt = filename;
	else
		stt++;

	*len = 0;

	*h = 0;
	while (*stt) {
		*h *= 33;
		*h += *stt++;
		++*len;
	}
} // }}}

void* key_alloc(struct discriminant_t* d, size_t* size) { // {{{
	return calloc(1, *size = key_size(d));
} // }}}

void* key_new(struct discriminant_t* d, struct stat* st, const char* filename, digest_t* digest, size_t* size) { // {{{

	long* ret = (long*)key_alloc(d, size);
	long* plong = ret;
	long _basename_h = 0;
	long _basename_len = 0;

	plong++;

	if (d->methods & DISC_DEV)
		*plong++ = st->st_dev;

	if (d->methods & DISC_SIZE)
		*plong++ = st->st_size;

	if (d->methods & DISC_MTIME)
		*plong++ = st->st_mtime;

	if (d->methods & DISC_USER)
		*plong++ = st->st_uid;

	if (d->methods & DISC_GROUP)
		*plong++ = st->st_gid;

	if (d->methods & DISC_PERMS)
		*plong++ = st->st_mode;

	if (d->methods & DISC_BASENAME) {
		basename_discriminant(filename, &_basename_h, &_basename_len);
		*plong++ = _basename_h;
		*plong++ = _basename_len;
	}

	char* pchar = (char*)plong;

	if (d->methods & DISC_MD5) {
		memcpy(pchar, digest->md5, sizeof(digest->md5));
		pchar += sizeof(digest->md5);
	}

	if (d->methods & DISC_SHA1) {
		memcpy(pchar, digest->sha1, sizeof(digest->sha1));
		pchar += sizeof(digest->sha1);
	}

	if (d->methods & DISC_SHA224) {
		memcpy(pchar, digest->sha224, sizeof(digest->sha224));
		pchar += sizeof(digest->sha224);
	}

	if (d->methods & DISC_SHA256) {
		memcpy(pchar, digest->sha256, sizeof(digest->sha256));
		pchar += sizeof(digest->sha256);
	}

	if (d->methods & DISC_SHA384) {
		memcpy(pchar, digest->sha384, sizeof(digest->sha384));
		pchar += sizeof(digest->sha384);
	}

	if (d->methods & DISC_SHA512) {
		memcpy(pchar, digest->sha512, sizeof(digest->sha512));
		pchar += sizeof(digest->sha512);
	}

	if (d->methods & DISC_RIPEMD160) {
		memcpy(pchar, digest->ripemd160, sizeof(digest->ripemd160));
		pchar += sizeof(digest->ripemd160);
	}

	ret[0] = pchar - (char*)ret; // size in bytes

	if (verbose() > 2) {
		int i;

		if (verbose() > 3)
			debug("key(%u)=%s", (unsigned)*size, bin2hex(ret, *size));

		if (d->methods & DISC_DEV)
			debug("dev=%lx\n", (unsigned long)st->st_dev);

		if (d->methods & DISC_SIZE)
			debug("size=%lu\n", (unsigned long)st->st_size);

		if (d->methods & DISC_MTIME)
			debug("mtime=%lu\n", (unsigned long)st->st_mtime);

		if (d->methods & DISC_USER)
			debug("uid=%ld\n", (long)st->st_uid);

		if (d->methods & DISC_GROUP)
			debug("gid=%ld\n", (long)st->st_gid);

		if (d->methods & DISC_SIZE)
			debug("perms=%o\n", st->st_mode);

		if (d->methods & DISC_BASENAME)
			debug("basename=%ld,%lx\n", _basename_h, _basename_len);

		char* _end = strdup("");
		if (d->end)
			asprintf(&_end, ":%llu", d->end);

		if (d->methods & DISC_MD5)
			debug("md5%s=%s", _end, bin2hex(digest->md5, MD5_DIGEST_LENGTH));

		if (d->methods & DISC_SHA1)
			debug("sha1%s=%s", _end, bin2hex(digest->sha1, sizeof(digest->sha1)));

		if (d->methods & DISC_SHA224)
			debug("sha224%s=%s", _end, bin2hex(digest->sha224, sizeof(digest->sha224)));

		if (d->methods & DISC_SHA256)
			debug("sha256%s=%s", _end, bin2hex(digest->sha256, sizeof(digest->sha256)));

		if (d->methods & DISC_SHA384)
			debug("sha384%s=%s", _end, bin2hex(digest->sha384, sizeof(digest->sha384)));

		if (d->methods & DISC_SHA512)
			debug("sha512%s=%s", _end, bin2hex(digest->sha512, sizeof(digest->sha512)));

		if (d->methods & DISC_RIPEMD160)
			debug("ripemd160%s=%s", _end, bin2hex(digest->ripemd160, sizeof(digest->ripemd160)));

		free(_end);
	}

	return ret;
} // }}}

void key_delete(void* key) { // {{{
	free(key);
} // }}}

