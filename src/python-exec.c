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
const size_t max_epython_len = 30;

static int try_env(char* bufp, const char* variable)
{
	const char* epython = getenv(variable);

	if (epython)
	{
		if (strlen(epython) <= max_epython_len)
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
		size_t rd = fread(bufp, 1, max_epython_len, f);

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
	size_t rd = readlink(path, bufp, max_epython_len);

	/* [max_epython_len] could mean that the name is too long */
	if (rd > 0 && rd < max_epython_len)
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

	/* 2 for the hyphen and the null terminator */
	if (len + max_epython_len + 2 >= BUFSIZ)
	{
		bufp = malloc(len + max_epython_len + 2);
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
