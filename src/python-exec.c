/* python-exec -- a Gentoo tool to choose the correct Python script
 * variant for currently selected Python implementation.
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

const char* const python_impls[] = { PYTHON_IMPLS };

static int try_env(char* bufp, const char* variable)
{
	const char* epython = getenv(variable);

	if (epython)
	{
		if (strlen(epython) <= 30)
		{
			strcpy(bufp, epython);
			return 1;
		}
		else
			fprintf(stderr, "EPYTHON value invalid (too long).\n");
	}

	return 0;
}

static int try_file(char* bufp, const char* path)
{
	FILE* f = fopen(path, "r");

	if (f)
	{
		size_t rd = fread(bufp, 1, 30, f);

		if (rd > 0 && feof(f))
		{
			bufp[rd] = 0;
			if (bufp[rd-1] == '\n')
				bufp[rd-1] = 0;
		}

		fclose(f);
	}

	return !!f;
}

static int try_symlink(char* bufp, const char* path)
{
	size_t rd = readlink(path, bufp, 30);

	/* 30 could mean that the name is too long */
	if (rd > 0 && rd < 30)
	{
		bufp[rd] = 0;
		return 1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	const char* const* i;
	char buf[BUFSIZ];
	char* bufp = buf;
	char* bufpy;

	size_t len = strlen(argv[0]);

	if (len + 32 >= BUFSIZ)
	{
		bufp = malloc(len + 32);
		if (!bufp)
		{
			fprintf(stderr, "%s: memory allocation failed (program name too long).\n",
					argv[0]);
			return 1;
		}
	}
	memcpy(bufp, argv[0], len);
	bufp[len] = '-';

	bufpy = &bufp[len+1];

	if (try_env(bufpy, "EPYTHON"))
		execvp(bufp, argv);
	if (try_file(bufpy, "/etc/env.d/python/config"))
		execvp(bufp, argv);
	if (try_symlink(bufpy, "/usr/bin/python2"))
		execvp(bufp, argv);
	if (try_symlink(bufpy, "/usr/bin/python3"))
		execvp(bufp, argv);

	for (i = python_impls; *i; ++i)
	{
		strcpy(&bufp[len+1], *i);
		execvp(bufp, argv);
	}

	if (bufp != buf)
		free(bufp);
	fprintf(stderr, "%s: no supported Python implementation variant found!\n",
			argv[0]);
	return 127;
}
