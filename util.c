/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2021 dogeystamp <dogeystamp@disroot.org>
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

void
logMsg(int lvl, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	if (lvl <= logLevel)
		vfprintf(stderr, msg, args);

	va_end(args);

	if (!lvl)
		exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);

	if (!p)
		logMsg(0, "Error allocating memory.\n");

	return p;
}

void *
erealloc(void *p, size_t size)
{
	p = realloc(p, size);

	if (!p)
		logMsg(0, "Error reallocating memory.\n");

	return p;
}

char *
san(char *str, int rep)
{
	if (!str)
		return "";
	if (!rep)
		return str;

	unsigned long long int len = strlen(str);
	unsigned long long int offset = 0;

	len = len > 255 ? 255 : len;

	char *dup = ecalloc(len + 1, sizeof(char));
	strcpy(dup, str);

	for (unsigned long long int i = 0; i < len; i++) {
		char c = dup[i];

		if ((c >= 'a' && c <= 'z') ||
		        (c >= 'A' && c <= 'Z') ||
		        (c >= '0' && c <= '9') ||
		        (c == '.' && i - offset != 0) ||
		        c == '-' || c == '_' ||
		        c == ' ')
			dup[i - offset] = dup[i];
		else
			offset++;
	}

	dup[len-offset] = '\0';

	return dup;
}

char fsep()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}
