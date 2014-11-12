
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

#define _GNU_SOURCE
#include "config.h"
#include "pathdb.h"
#include "ionice.h"
#include "options.h"
#include "digest.h"
#include "state.h"
#include "memory.h"
#include "error.h"
#include "string.h"
#include "discriminant.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>


void help() { // {{{
	puts(
#include "help.ci"
  );
} // }}}

char* tempfile(char* base) {
	char* tmp = NULL;
	struct timeval now;
	gettimeofday(&now, NULL);
	asprintf(&tmp, "%s.%d.%lu.%lu.%x", base, getpid(), now.tv_sec, now.tv_usec, rand());
	return tmp;
}

void _process_file(const char* filename, struct stat* _st) { // {{{

	CACHED_CONFIG(cfg);
	CACHED_STATE(st);

	// if <dev,ino> is already in the set, we already scanned the hardlink to this file
	devino_t devino;
	file_t* file = NULL;
	file_t* found = NULL;
	cluster_t* cluster = NULL;
	size_t keylen = 0;
	long* key = NULL;

	devino_init(&devino, _st->st_dev, _st->st_ino);

	if (link_type_is_symb(cfg->flags) && (filename[0] != '/'))
		fatal("Merging via symlinks with relative paths is not an option.\n");

	if (!_st->st_size) {
		debug("\tFile %s empty, ignoring.", filename);
		return;
	}

	if (devino2file_find(&st->filesByDevIno, &devino, &found) == HTABLE_FOUND) {
		cluster = found->cluster;
		key = found->key;
		file = file_new(filename, _st, found->cluster);
		clfiles_add(&cluster->files, strdup(filename), strlen(filename)+1, file);
		debug("\tFile %s (%lu bytes): found in devino (dev=%x, ino=%ld)", filename, _st->st_size,
				devino.dev, devino.inode);

	} else {

		digest_t digest;

		if (digest_file(filename, _st, &digest) < 0)
			goto error;

		struct discriminant_t* _disc = current_discriminant();

		key = (long*)key_new(_disc, _st, filename, &digest, &keylen);

		/* Check: Already have a file with the same key? */
		if (key2cluster_find(&st->clustersByKey, (long*)key, keylen, &cluster) == HTABLE_FOUND) {
			file = file_new(filename, _st, cluster);
			devino2file_add(&st->filesByDevIno, xmemdup(&devino, sizeof(devino)), file);
			clfiles_add(&cluster->files, strdup(filename), strlen(filename)+1, file);
			debug("\tFile %s (%lu bytes): added to cluster (dev=%x, ino=%ld, key=%s)", filename, _st->st_size,
					devino.dev, devino.inode, bin2hex(key+1, key[0]-sizeof(key[0])));
			free(key);
			key = NULL;

		} else {
			cluster = cluster_new();
			file = file_new(filename, _st, cluster);
			file->key = key;
			clfiles_add(&cluster->files, strdup(filename), strlen(filename)+1, file);
			devino2file_add(&st->filesByDevIno, xmemdup(&devino, sizeof(devino)), file);
			key2cluster_add(&st->clustersByKey, key, keylen, cluster);
			debug("\tFile %s (%lu bytes): new cluster (dev=%x, ino=%ld, key=%s)", filename, _st->st_size,
					devino.dev, devino.inode, bin2hex(key+1, key[0]-sizeof(key[0])));

		}
	}

	return;

error:
	if (file) {
		file_delete(file);
		file = NULL;
	}
} // }}}

void process_file(const char* s, struct stat* _st) { // {{{

	struct stat __st;
	struct stat* st = _st;

	if (!st) {
		st = &__st;
		if (lstat(s, st) < 0)
			error("Error accessing \"%s\": %s\n", s, strerror(errno));
	}

	if (!S_ISREG(st->st_mode))
		return;

	CACHED_CONFIG(cfg);

	if (cfg->minage && (time(NULL) - st->st_mtime < cfg->minage)) {
		debug("Ignoring file too young (age=%ld secs, file=%s).\n",
				time(NULL) - st->st_mtime, s);
		return;
	}

	_process_file(s, st);

} // }}}

void run_buf(char* buf, size_t* buf_len, char delim) { // {{{
	char* start = buf;

	while (start < buf + *buf_len) {
		char* end = start;
		int len = 0;
		while ((*end != delim) && (len < *buf_len)) {
			++end;
			++len;
		}

		if (*end == delim) {
			char* _t = strndup(start, end-start);
			process_file(_t, NULL);
			start = end+1;
			free(_t);
		}
	}

	if (start == buf)
		fatal("Path too long?\n");

	*buf_len -= start - buf;
	memmove(buf, start, *buf_len);
} // }}}

void _find(const char* path) { // {{{

	struct stat st;
	if (lstat(path, &st) < 0)
		error("Error accessing \"%s\": %s\n", path, strerror(errno));
	
	if (S_ISLNK(st.st_mode))
		return;

	if (S_ISREG(st.st_mode))
		process_file(path, &st);

	if (S_ISDIR(st.st_mode)) {

		DIR* dh = opendir(path);

		if (!dh) {
			error("Error opening \"%s\": %s\n", path, strerror(errno));
			return;
		}

		struct dirent* de;

		int _found_dot = 0;
		int _found_dot2 = 0;

		while ((de = readdir(dh))) {

			if (!_found_dot && !strcmp(de->d_name, ".")) {
				++_found_dot;
				continue;
			}

			if (!_found_dot2 && !strcmp(de->d_name, "..")) {
				++_found_dot2;
				continue;
			}

			char* full = NULL;
			asprintf(&full, "%s/%s", path, de->d_name);

			_find(full);

			free(full);

		}

		closedir(dh);
	}

	// char/block devices and FIFOs are ignored
} // }}}

void process_path(const char* path) { // {{{
	struct config_t* cfg = config();

	debug("Processing path %s", path);

	_find(path);
} // }}}

int showFile(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	char* filename = (char*)key;
	assert(strlen(filename)+1 == keylen);

	debug("file=%s", filename);
	return 0;
}

void showCluster(cluster_t* cluster, long* lkey) {

	debug("cluster[%s]: %lu %s",
		bin2hex(lkey+1, lkey[0] - sizeof(lkey[0])),
		(unsigned long)cluster->files.entries,
		cluster->files.entries ? "files" : "file");

	htable_foreach(&cluster->files, showFile, NULL);

}

int fileStep(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	char* filename = (char*)key;
	file_t* file = (file_t*)data;	
	run_state* prev = (run_state*)cbdata;

	assert(strlen(filename)+1 == keylen);

	_process_file(filename, &file->st);

	return 0;
}

int fileNameClean(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	char* filename = (char*)key;
	file_t* file = (file_t*)data;	
	run_state* prev = (run_state*)cbdata;

	assert(strlen(filename)+1 == keylen);

	file_delete(file);
	free(filename);

	return 0;
}

int fileDevInoClean(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	devino_t* devino = (devino_t*)key;
	file_t* found = NULL;
	run_state* prev = (run_state*)cbdata;

	free(devino);
	/* This is freed by fileNameClean above */
	/* file_delete(found); */

	return 0;
}

int clusterClean(void* key, size_t keylen, cluster_t* cluster, run_state* prev) {

	long* lkey = (long*)key;

	assert(lkey[0] == keylen);
	assert(cluster);

	htable_foreach(&cluster->files, fileNameClean, prev);

	cluster_delete(cluster);
	key_delete(key);

	return 0;
}

int clusterStep(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	long* lkey = (long*)key;
	cluster_t* cluster = (cluster_t*)data;
	run_state* prev = (run_state*)cbdata;

	debug("Processing cluster[%s]:", bin2hex(lkey+1, lkey[0] - sizeof(lkey[0])));

	assert(lkey[0] == keylen);
	assert(dlen = sizeof(*cluster));
	assert(cluster);

	if (verbose() > 2)
		showCluster(cluster, lkey);

	if (cluster->files.entries > 1)
		htable_foreach(&cluster->files, fileStep, prev);

	clusterClean(key, keylen, cluster, prev);

	return 0;
}

int xlink(const char* _oldname, const char* _newname) {

	debug("\t\tlink %s <- %s\n", _oldname, _newname);

	if (dryrun())
		return 0;

	return link(_oldname, _newname);
}

int xrename(const char* _oldname, const char* _newname) {

	debug("\t\trename %s -> %s\n", _oldname, _newname);

	if (dryrun())
		return 0;

	return rename(_oldname, _newname);
}

int xsymlink(const char* _oldname, const char* _newname) {

	debug("\t\tsymlink %s <- %s\n", _oldname, _newname);

	if (dryrun())
		return 0;

	return symlink(_oldname, _newname);
}

int xunlink(const char* filename) {
	debug("\t\tunlink %s\n", filename);

	if (dryrun())
		return 0;

	return unlink(filename);
}

void maybeReport(const char* filename, size_t filename_len, int ifile) {

	static int _first_call = 1;

	CACHED_CONFIG(cfg);

	if (cfg->report_fd < 0)
		return;

	static struct iovec _iov[3] = {
		{ "\0", 1 },
		{ "\0", 1 },
		{ "\0", 1 }
	};

	static char _separator[2] = "a";

	if (_first_call) {
		_separator[0] = cfg->report_separator;
		_iov[0].iov_base = &_separator[0];
		_iov[2].iov_base = &_separator[0];
	}

	_iov[1].iov_base = (void*)filename;
	_iov[1].iov_len = filename_len;

	int off = 1;
	if (_first_call)
		off = 1;

	else if (!ifile)
		off = 0;

	writev(cfg->report_fd, _iov + off, 3 - off);
	
	_first_call = 0;

}

int fileMergeStep(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	char* filename = (char*)key;
	file_t* file = (file_t*)data;	
	int* ifile = (int*)cbdata;

	static char* baseFile = NULL;
	static file_t* baseFileT = NULL;

	CACHED_CONFIG(cfg);
	CACHED_STATE(_state);

	assert(strlen(filename)+1 == keylen);

	maybeReport(filename, keylen-1, *ifile);

	if (!*ifile) {
		baseFile = filename;
		baseFileT = file;
		debug("Merging: base: %s\n", baseFile);
		ifile[0]++;
		return 0;
	}

	if ((file->st.st_ino == baseFileT->st.st_ino) &&
	    (file->st.st_dev == baseFileT->st.st_dev)) {
		debug("\tIgnoring, same inode: %s <- %s\n", baseFile, filename);

	} else if (link_type_is_symb(cfg->flags)) {
		debug("\tsymlink: %s <- %s\n", baseFile, filename);
		char* tmp = tempfile(filename);
		struct stat st;

		if (stat(filename, &st) < 0) {
			error("Could not stat %s: %s\n", filename, strerror(errno));

		} else if (xrename(filename, tmp) < 0) {
			error("Could not rename %s to %s: %s\n", filename, tmp, strerror(errno));

		} else if (xsymlink(baseFile, filename) < 0) {
			error("Could not symlink %s to %s: %s\n", filename, baseFile, strerror(errno));
			if (rename(tmp, filename) < 0)
				error("WARNING: dangling temporary file %s (should be %s)\n",
					tmp, filename);

		} else if (xunlink(tmp) < 0) {
			error("Could not unlink %s%s\n", tmp, strerror(errno));
			error("WARNING: dangling temporary file %s (should be removed)\n");

		} else if (st.st_nlink == 1) {
			_state->saved += st.st_size;

		}

		free(tmp);

	} else if (link_type_is_hard(cfg->flags)) {
		debug("\tlink: %s <- %s\n", baseFile, filename);
		char* tmp = tempfile(filename);

		struct stat st;

		if (stat(filename, &st) < 0) {
			error("Could not stat %s: %s\n", filename, strerror(errno));

		} else if (xlink(filename, tmp) < 0) {
			error("Could not create temporary link %s to %s: %s\n", tmp, filename, strerror(errno));

		} else if (xunlink(filename) < 0) {
			error("Could not remove %s: %s\n", filename, strerror(errno));
			unlink(tmp);

		} else if (xlink(baseFile, filename) < 0) {
			error("Could not link %s to %s: %s\n", filename, baseFile, strerror(errno));
			if (rename(tmp, filename) < 0)
				error("WARNING: MUST rename %s to %s.\n", tmp, filename);
			
		} else if (xunlink(tmp) < 0) {
			error("WARNING: MUST remove dangling temporary file %s.\n", tmp);

		} else if (st.st_nlink == 1) {
			_state->saved += st.st_size;

		}

		free(tmp);

	} else {
		fatal("INTERNAL ERROR %s:%d.", __FILE__, __LINE__);

	}

	ifile[0]++;
	return 0;
}

int mergeCluster(void* key, size_t keylen, void* data, size_t dlen, void* cbdata) {

	long* lkey = (long*)key;
	cluster_t* cluster = (cluster_t*)data;

	if (cluster->files.entries > 1) {
		int ifile = 0;
		htable_foreach(&cluster->files, fileMergeStep, &ifile);
	}

	return 0;
}

void run() { // {{{
	
	struct config_t* cfg = config();

	if (path_source_is_stdin(cfg->flags)) {

		char delim = path_source_is_stdin0(cfg->flags) ? '\0' : '\n'; // nul terminated

		static char buf[4096];
		size_t buf_len = 0; // number of bytes held

		ssize_t nread;

		while (1) {

			while ((nread = read(0, buf + buf_len, sizeof(buf) - buf_len)) > 0) {

				buf_len += nread;

				run_buf(buf, &buf_len, delim);

			}

			if ((nread < 0) && (errno == EINTR))
				continue;
			break;
		}

	} else {

		foreach_path(process_path);

	}

	run_state prev;

	while (1) {

		if (!state_next_step(&prev))
			break;

		htable_foreach(&prev.clustersByKey, clusterStep, &prev);

		htable_foreach(&prev.filesByDevIno, fileDevInoClean, &prev);

		htable_destroy(&prev.filesByDevIno);
		htable_destroy(&prev.clustersByKey);

	}

	htable_foreach(&state()->clustersByKey, mergeCluster, NULL);

} // }}}

void cgroup_init(const char* cgroup) { // {{{

	char* cg_file = NULL;
	asprintf(&cg_file, "%s/tasks", cgroup);

	int fd = open(cg_file, O_RDWR);
	if (fd < 0)
		fatal("Could open cgroup file %s: %s\n", cg_file, strerror(errno));

	long pid = getpid();
	char b[22];
	int n = snprintf(b, sizeof(b), "%lu\n", pid);

	if (n >= sizeof(b))
		fatal("INTERNAL ERRROR: pid too large!?");

	n = write(fd, b, strlen(b));

	if (n != strlen(b))
		fatal("Could not add %lu into cgroup %s: %s", pid, cgroup, strerror(errno));

	close(fd);
	free(cg_file);

} // }}}

void setup() { // {{{

	struct config_t* cfg = config();

	if (cfg->nice > 0)
		nice(cfg->nice);

	ionice_setup();

	int icg = 0;
	while (icg < cfg->cgroupc)
		cgroup_init(cfg->cgroups[icg++]);

	state_setup();
} // }}}

int main(int argc, char* argv[]) { // {{{

	parse_options(argc, argv);

	setup();

	run();

	digest_clean();

	fprintf(stdout, "saved=%llu\n", (unsigned long long)state()->saved);

	return 0;
} // }}}

