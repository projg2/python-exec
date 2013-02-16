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

/* All possible EPYTHON values, provided to the configure script. */
const char* const python_impls[] = { PYTHON_IMPLS };
/* Maximum length of an EPYTHON value. */
const size_t max_epython_len = MAX_EPYTHON_LEN;

/**
 * Try to obtain EPYTHON from an environment variable.
 *
 * @bufp points to the space in the buffer where the value shall be
 * written (first byte after the hyphen). The buffer must have at least
 * max_epython_len space.
 *
 * @variable contains the environment variable name.
 *
 * Returns 1 on success, 0 otherwise.
 */
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

/**
 * Try to read EPYTHON from a regular file.
 *
 * @bufp points to the space in the buffer where the value shall be
 * written (first byte after the hyphen). The buffer must have at least
 * max_epython_len space.
 *
 * @variable contains the file path.
 *
 * Returns 1 on success, 0 otherwise.
 */
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

/**
 * Try to obtain EPYTHON from a symlink target.
 *
 * @bufp points to the space in the buffer where the value shall be
 * written (first byte after the hyphen). The buffer must have at least
 * max_epython_len space.
 *
 * @variable contains the symlink path.
 *
 * Returns 1 on success, 0 otherwise.
 */
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

/**
 * Shift the argv array one element left. Left-most element will be
 * removed, right-most will be replaced with trailing NULL.
 *
 * If argc is used, it has to be decremented separately.
 *
 * @argv is a pointer to first argv element.
 */
static void shift_argv(char* argv[])
{
	char** i;

	for (i = argv; *i; ++i)
		i[0] = i[1];
}

/**
 * Usage: python-exec <script> [<argv>...]
 *
 * python-exec tries to execute <script> with most preferred Python
 * implementation supported by it. It determines whether a particular
 * implementation is supported through appending '-${EPYTHON}'
 * to the script path.
 */
int main(int argc, char* argv[])
{
	const char* const* i;
	char buf[BUFSIZ];
	char* bufp = buf;
	char* bufpy;

	const char* script = argv[1];

	if (!script)
	{
		fprintf(stderr, "Usage: %s <script>\n", argv[0]);
		return EXIT_FAILURE;
	}

	{
		size_t len = strlen(script);

		/* Check whether our stack buffer is large enough. If necessary,
		 * allocate a new one from the heap.
		 *
		 * 2 is for the hyphen and the null terminator.
		 */
		if (len + max_epython_len + 2 >= BUFSIZ)
		{
			bufp = malloc(len + max_epython_len + 2);
			if (!bufp)
			{
				fprintf(stderr, "%s: memory allocation failed (program name too long).\n",
						script);
				return EXIT_FAILURE;
			}
		}
		memcpy(bufp, script, len);
		bufp[len] = '-';

		shift_argv(argv);

		bufpy = &bufp[len+1];

		/**
		 * The implementation check order:
		 * 1) environment variable EPYTHON (local choice),
		 * 2) eselect-python main Python interpreter,
		 * 3) eselect-python Python 2 & Python 3 choices,
		 * 4) any of the supported implementations.
		 *
		 * For 3), the order is basically irrelevant since whichever
		 * is preferred will be tried in 2) anyway.
		 *
		 * 4) uses the eclass-defined order.
		 */
		if (try_env(bufpy, "EPYTHON"))
			execvp(bufp, argv);
		if (try_file(bufpy, EPREFIX "/etc/env.d/python/config"))
			execvp(bufp, argv);
		if (try_symlink(bufpy, EPREFIX "/usr/bin/python2"))
			execvp(bufp, argv);
		if (try_symlink(bufpy, EPREFIX "/usr/bin/python3"))
			execvp(bufp, argv);

		for (i = python_impls; *i; ++i)
		{
			strcpy(&bufp[len+1], *i);
			execvp(bufp, argv);
		}
	}

	/* If no execvp() succeeded, that means we either don't have
	 * a single supported implementation here or something is seriously
	 * broken.
	 */
	if (bufp != buf)
		free(bufp);
	fprintf(stderr, "%s: no supported Python implementation variant found!\n",
			script);
	return 127;
}
