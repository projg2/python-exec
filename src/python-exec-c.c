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
#include <errno.h>

#ifdef HAVE_READLINK
#	include <unistd.h>
#	include <limits.h>
#	include <sys/stat.h>
#endif

/* All possible EPYTHON values, provided to the configure script. */
const char* const python_impls[] = { PYTHON_IMPLS };
/* Maximum length of an EPYTHON value. */
const size_t max_epython_len = MAX_EPYTHON_LEN;

const char path_sep = '/';

/**
 * Try to obtain the value of an environment variable.
 *
 * @bufp points to the space in the buffer where the value shall be
 * written (first byte after the hyphen).
 *
 * @variable contains the environment variable name.
 *
 * @max_len specifies the maximum value length. The buffer must have
 * at least one byte more for the null terminator.
 *
 * Returns 1 on success, 0 otherwise.
 */
static int try_env(char* bufp, const char* variable, size_t max_len)
{
	const char* epython = getenv(variable);

	if (epython)
	{
		if (strlen(epython) <= max_len)
		{
			strcpy(bufp, epython);
			return 1;
		}
		else
			fprintf(stderr, "${%s} value invalid (too long).\n",
					variable);
	}

	return 0;
}

/**
 * Try to read contents of a regular file.
 *
 * @bufp points to the space in the buffer where the value shall be
 * written (first byte after the hyphen).
 *
 * @variable contains the file path.
 *
 * @max_len specifies the maximum value length. The buffer must have
 * at least one byte more for the null terminator.
 *
 * Returns 1 on success, 0 otherwise.
 */
static int try_file(char* bufp, const char* path, size_t max_len)
{
	FILE* f = fopen(path, "r");

	if (f)
	{
		size_t rd = fread(bufp, 1, max_len, f);

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

#ifdef HAVE_READLINK

/**
 * Try to read a symlink target.
 *
 * @bufp points to the space in the buffer where the target shall be
 * written (first byte after the hyphen).
 *
 * @variable contains the symlink path.
 *
 * @max_len specifies the maximum value length. The buffer must have
 * at least one byte more for the null terminator.
 *
 * Returns 1 on success, 0 otherwise.
 */
static int try_symlink(char* bufp, const char* path, size_t max_len)
{
	size_t rd;

	errno = 0;
	/* 1 for the null terminator with max length */
	rd = readlink(path, bufp, max_len + 1);

	/* [max_len] could mean that the name is too long */
	if (rd > 0 && rd < max_len + 1)
	{
		bufp[rd] = 0;
		return 1;
	}

	return 0;
}

#endif

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
 * Check the buffer size and reallocate it if necessary.
 *
 * Assumes that buffers <= BUFSIZ are stack-allocated,
 * and heap-allocated above that size.
 *
 * @buf is the pointer to the buffer address holder.
 *
 * @buf_size is the pointer to the buffer size holder.
 *
 * @req_size is the requested buffer size.
 *
 * Returns 1 on success, or 0 if memory allocation failed.
 * In the latter case, buffer is left unchanged.
 */
static int expand_buffer(char** buf, size_t* buf_size, size_t req_size)
{
	if (req_size >= *buf_size)
	{
		char *new_buf;

		/* first alloc */
		if (*buf_size <= BUFSIZ)
			new_buf = malloc(req_size);
		else /* realloc */
			new_buf = realloc(*buf, req_size);

		if (!new_buf)
			return 0;
		*buf = new_buf;
		*buf_size = req_size;
	}

	return 1;
}

#ifdef HAVE_READLINK

/**
 * Obtain symlink length. Assumes that symlinks don't change during
 * the process.
 *
 * @path contains the path to the symlink.
 *
 * Returns the symlink length or 0 if the file is not a symlink.
 */
size_t get_symlink_length(const char* path)
{
	struct stat st;

	errno = 0;
	if (!lstat(path, &st) && S_ISLNK(st.st_mode))
	{
		/* how are we supposed to read that? */
		if (st.st_size > SSIZE_MAX)
		{
			errno = EINVAL;
			return 0;
		}

		return st.st_size;
	}

	return 0;
}

#endif

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
	size_t buf_size = sizeof(buf);
	char* bufp = buf;
	char* bufpy;

	const char* script = argv[1];
	int symlink_resolution = 0;

	if (!script || !script[0])
	{
		fprintf(stderr, "Usage: %s <script>\n", argv[0]);
		return EXIT_FAILURE;
	}

	shift_argv(argv);

	while (1)
	{
		size_t len;
		char* fnpos;

		if (!symlink_resolution)
			len = strlen(script);
#ifdef HAVE_READLINK
		else
		{
			size_t sym_len = get_symlink_length(bufp);

			if (!sym_len)
			{
				if (errno != 0)
					fprintf(stderr, "%s: unable to stat symlink at %s: %s\n",
							script, bufp, strerror(errno));
				else /* no more symlinks to try */
					fprintf(stderr, "%s: no supported Python implementation variant found!\n",
							script);
				break;
			}

			fnpos = strrchr(bufp, path_sep);
			if (fnpos)
				len = &fnpos[1] - bufp;
			else
				len = 0;

			len += sym_len;
		}
#endif

		/* 2 is for the hyphen and the null terminator. */
		if (!expand_buffer(&bufp, &buf_size, len + max_epython_len + 2))
		{
			fprintf(stderr, "%s: memory allocation failed (program name too long).\n",
					script);
			break;
		}

		if (!symlink_resolution)
			memcpy(bufp, script, len);
#ifdef HAVE_READLINK
		else
		{
			/**
			 * In order to support relative symlinks, preserve
			 * the current directory (but strip filename).
			 */
			if (!fnpos)
				fnpos = bufp;
			else
				++fnpos;

			if (!try_symlink(fnpos, bufp, len))
			{
				fprintf(stderr, "%s: unable to read symlink at %s: %s.\n",
						script, bufp,
						errno != 0 ? strerror(errno) : "target length changed");
				break;
			}

			/* Symlink is absolute, move the path. */
			if (*fnpos == path_sep && fnpos != bufp)
				memmove(bufp, fnpos, strlen(fnpos) + 1);
		}
#endif

		bufpy = &bufp[len+1];
		bufpy[-1] = '-';

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
		if (try_env(bufpy, "EPYTHON", max_epython_len))
			execvp(bufp, argv);
		if (try_file(bufpy, EPREFIX "/etc/env.d/python/config", max_epython_len))
			execvp(bufp, argv);
#ifdef HAVE_READLINK
		if (try_symlink(bufpy, EPREFIX "/usr/bin/python2", max_epython_len))
			execvp(bufp, argv);
		if (try_symlink(bufpy, EPREFIX "/usr/bin/python3", max_epython_len))
			execvp(bufp, argv);
#endif

		for (i = python_impls; *i; ++i)
		{
			strcpy(bufpy, *i);
			execvp(bufp, argv);
		}

		/**
		 * Strip the hyphen back and try symlink resolution.
		 */
		bufpy[-1] = 0;

#ifdef HAVE_READLINK
		symlink_resolution = 1;
#else
		break;
#endif
	}

	/* If no execvp() succeeded, that means we either don't have
	 * a single supported implementation here or something is seriously
	 * broken.
	 */
	if (bufp != buf)
		free(bufp);
	return 127;
}
