
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

#include "digest.h"
#include "discriminant.h"
#include "state.h"
#include "config.h"
#include "error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>

typedef struct digest_mds {
	int digest_mask;
	const EVP_MD *md5;
	const EVP_MD *sha1;
	const EVP_MD *sha224;
	const EVP_MD *sha256;
	const EVP_MD *sha384;
	const EVP_MD *sha512;
	const EVP_MD *ripemd160;
} digest_mds;

digest_mds _mds;

void digest_setup(int disc_mask) { // {{{

	OpenSSL_add_all_digests();

	memset(&_mds, 0, sizeof(_mds));

	_mds.digest_mask = disc_mask & DISC_CONTENT_MASK;

	if (_mds.digest_mask & DISC_MD5)
		_mds.md5 = EVP_md5();

	if (_mds.digest_mask & DISC_SHA1)
		_mds.sha1 = EVP_sha1();

	if (_mds.digest_mask & DISC_SHA224)
		_mds.sha224 = EVP_sha224();

	if (_mds.digest_mask & DISC_SHA256)
		_mds.sha256 = EVP_sha256();

	if (_mds.digest_mask & DISC_SHA384)
		_mds.sha384 = EVP_sha384();

	if (_mds.digest_mask & DISC_SHA512)
		_mds.sha512 = EVP_sha512();

	if (_mds.digest_mask & DISC_RIPEMD160)
		_mds.ripemd160 = EVP_ripemd160();

} // }}}

int digest_init(digest_state_t* s) { // {{{

	int digestc = 0;

	memset(s, 0, sizeof(*s));

	if (_mds.digest_mask & DISC_MD5) {
		s->md5 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->md5, _mds.md5, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_SHA1) {
		s->sha1 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->sha1, _mds.sha1, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_SHA224) {
		s->sha224 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->sha224, _mds.sha224, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_SHA256) {
		s->sha256 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->sha256, _mds.sha256, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_SHA384) {
		s->sha384 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->sha384, _mds.sha384, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_SHA512) {
		s->sha512 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->sha512, _mds.sha512, NULL);
		++digestc;
	}

	if (_mds.digest_mask & DISC_RIPEMD160) {
		s->ripemd160 = EVP_MD_CTX_create();
		EVP_DigestInit_ex(s->ripemd160, _mds.ripemd160, NULL);
		++digestc;
	}

	return digestc;
} // }}}

void digest_clean() {
	// no cleanup possible for _mds.* members?
}

void digest_update(digest_state_t* s, const void* b, size_t len) { // {{{

	if (s->md5)
		EVP_DigestUpdate(s->md5, b, len);

	if (s->sha1)
		EVP_DigestUpdate(s->sha1, b, len);

	if (s->sha224)
		EVP_DigestUpdate(s->sha224, b, len);

	if (s->sha256)
		EVP_DigestUpdate(s->sha256, b, len);

	if (s->sha384)
		EVP_DigestUpdate(s->sha384, b, len);

	if (s->sha512)
		EVP_DigestUpdate(s->sha512, b, len);

	if (s->ripemd160)
		EVP_DigestUpdate(s->ripemd160, b, len);

} // }}}

void digest_final(digest_state_t* s, digest_t* t) { // {{{

	unsigned int len;
       
	if (s->md5) {
		len = sizeof(t->md5);
		EVP_DigestFinal_ex(s->md5, t->md5, &len);
	}

	if (s->sha1) {
		len = sizeof(t->sha1);
		EVP_DigestFinal_ex(s->sha1, t->sha1, &len);
	}

	if (s->sha224) {
		len = sizeof(t->sha224);
		EVP_DigestFinal_ex(s->sha224, t->sha224, &len);
	}

	if (s->sha256) {
		len = sizeof(t->sha256);
		EVP_DigestFinal_ex(s->sha256, t->sha256, &len);
	}

	if (s->sha384) {
		len = sizeof(t->sha384);
		EVP_DigestFinal_ex(s->sha384, t->sha384, &len);
	}

	if (s->sha512) {
		len = sizeof(t->sha512);
		EVP_DigestFinal_ex(s->sha512, t->sha512, &len);
	}

	if (s->ripemd160) {
		len = sizeof(t->ripemd160);
		EVP_DigestFinal_ex(s->ripemd160, t->ripemd160, &len);
	}

} // }}}

int digest_file(const char* filename, struct stat* _st, struct digest_t* digest) { // {{{

	digest_state_t state;
	int digestc = digest_init(&state);

	CACHED_CONFIG(cfg);

	if (digestc) {

		int fd = open(filename, O_RDONLY);
		if (fd < 0) {
			error("Could not open \"%s\": %s.\n", filename, strerror(errno));
			return -1;
		}

		struct discriminant_t* disc = current_discriminant();

		if (cfg->read_policy == 'r') {

			static char* buf = NULL;
			if (!buf)
				buf = (char*)malloc(cfg->bufsize);

			off_t offset = 0;

			while (1) {
				size_t nbytes = cfg->bufsize;
				if (disc->end && ((offset + nbytes) > disc->end))
					nbytes = disc->end - offset;

				ssize_t nread = read(fd, buf, nbytes);

				if (nread < 0) {
					if (errno == EINTR)
						continue;
					error("Error on read from %s: %s\n.", filename, strerror(errno));
					return -1;
				}

				if (nread == 0)
					break;

				digest_update(&state, buf, nread);

				offset += nread;
			}
				
		} else if (cfg->read_policy == 'm') {

			off_t length = _st->st_size;
			off_t offset = 0;

			if (disc->end)
				length = disc->end;

			while (offset < length) {

				off_t ilen = cfg->bufsize;
				if (offset + ilen > length)
					ilen = length - offset;

				void* b = mmap(NULL, ilen, PROT_READ, MAP_SHARED /* ? */, fd, offset);

				digest_update(&state, b, ilen);

				munmap(b, ilen);
				offset += ilen;
			}

		}

		close(fd);

		digest_final(&state, digest);
	}

	return digestc;
} // }}}

