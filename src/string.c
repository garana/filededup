
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

#include "string.h"

char* bin2hex(void* b, size_t s) {
	static char c2h[] = "0123456789abcdef";
	unsigned char* c = (unsigned char*)b;

	static char* _r = NULL;

	char* ret = (char*)malloc(2*s + 1);
	char* r = ret;

	while (s--) {
		*r++ = c2h[c[0] >> 4];
		*r++ = c2h[c[0] & 0x0f];
		++c;
	}

	*r = '\0';

	if (_r)
		free(_r);

	_r = ret;
	return ret;
}

