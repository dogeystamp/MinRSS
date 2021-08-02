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

	char *dup = ecalloc(len + 1, sizeof(char));
	strcpy(dup, str);

	for (unsigned long long int i = 0; i < len; i++) {
		if ((dup[i] >= 'a' && dup[i] <= 'z') ||
			(dup[i] >= 'A' && dup[i] <= 'Z') ||
			(dup[i] >= '0' && dup[i] <= '9') ||
			 dup[i] == '-' || dup[i] == '_')
			dup[i - offset] = dup[i];
		else if (dup[i] == ' ')
			dup[i - offset] = '_';
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
