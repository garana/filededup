
filededup
=========

File deduplication tool.

## What it does

It merges duplicate files by using hardlinks.

## How duplicate files are detected?

This tool assumes that if two files share some criteria, then they have the same content, so they can be merged.

Which are the possible critera? They are actually configurable (See "filededup --help", look for "--eval"). A brief:

 * inode access flags (the "-rw-rw-r--" colum in "ls -l" output).
 * owner (user & group)
 * device number that holds the filesystem.
 * md5, sha1, sha256, sha512, ripemd160, etc of the file contents, and possibly only on the first N bytes.

The main idea is to detect duplicate files with the least possible amount of work.

By default, the processing is done like this:

1. All the files are grouped in clusters, where each cluster contains all the files that share all these:
  * size
  * perms
  * user
  * group
2. Then, each cluster is divided based on the sha1 of the first 4096 bytes.
3. Then, each cluster is divided again based on the sha512 AND ripemd160 of the hole file content.
4. Then, the files of each cluster are merged by using hardlinks.

Things that can be customized:
 * how/which steps are run,
 * digest functions allowed,
 * read function (read(2) vs mmap(2)), and buffer size,
 * nice level, ionice level, cgroup attachment,
 * "minimum file age" (don't merge files "younger" than, say, 2 hours).

## How to build?

Run

> cd src && make

And you should have filededup.

