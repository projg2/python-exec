/* python-exec -- a Gentoo tool to choose the correct Python script
 * variant for currently selected Python implementation.
 * (c) 2012-2016 Michał Górny
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

#ifndef BUFFER_SIZE
#	define BUFFER_SIZE BUFSIZ
#endif

/* Python script root directory */
const char python_scriptroot[] = PYTHON_SCRIPTROOT "/";
/* Maximum length of an EPYTHON value. */
const size_t max_epython_len = MAX_EPYTHON_LEN;

const char path_sep = '/';

enum python_impl_preference
{
	IMPL_DEFAULT = -1,
};

struct python_impl
{
	const char* name;
	int preference;
};

struct python_impl python_impls[] = {
	PYTHON_IMPLS
};

/**
 * Set path in scriptbuf for given impl.
 *
 * @bufp points to the buffer.
 *
 * @impl holds the implementation name.
 *
 * @progname contains the program basename.
 */
 static void set_scriptbuf(char* bufp, const char* impl,
		const char* progname)
{
	strcpy(bufp, impl);
	strcat(bufp, "/");
	strcat(bufp, progname);
}

/**
 * Set preference for implementation, if it is not set already.
 *
 * @impl Implementation to set preference for.
 *
 * @pref Requested preference value.
 *
 * Returns @pref if preference has been updated, the current preference
 * level if it has not or IMPL_DEFAULT if impl is unsupported.
 */
static int set_impl_preference(const char* impl, int pref)
{
	struct python_impl* i;

	for (i = python_impls; i->name; ++i)
	{
		if (!strcmp(impl, i->name))
		{
			if (i->preference == IMPL_DEFAULT)
				i->preference = pref;
			return i->preference;
		}
	}

	return IMPL_DEFAULT;
}

/**
 * Try to read implementation from a single-value file, and set its
 * preference to @pref.
 *
 * @path Path to file containing the preference.
 *
 * @pref Requested preference value.
 *
 * Returns 1 if preference has been updated, 0 if path could not be
 * read, contains an invalid implementation or the implementation has
 * non-default preference value set already.
 */
static int try_preference_from_file(const char* path, int pref)
{
	FILE* f = fopen(path, "r");
	/* +1 for '\n', +2 to enforce EOF */
	char buf[max_epython_len + 2];

	if (f)
	{
		size_t rd;

		rd = fread(buf, 1, sizeof(buf), f);
		if (rd > 0 && feof(f))
		{
			if (buf[rd-1] == '\n')
				--rd;
			/* ensure null termination */
			buf[rd] = 0;

			fclose(f);
			return set_impl_preference(buf, pref) == pref;
		}

		fclose(f);
	}

	return 0;
}

/**
 * Load configuration files and set implementation preferences
 * accordingly.
 */
static void load_configuration()
{
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
	int curr_pref = 0;
	const char* epython;

	epython = getenv("EPYTHON");
	if (epython)
	{
		if (set_impl_preference(epython, curr_pref) == curr_pref)
			++curr_pref;
		else
			fprintf(stderr, "EPYTHON value invalid (%s).\n", epython);
	}

	curr_pref += try_preference_from_file(
			EPREFIX "/etc/env.d/python/config", curr_pref);
	curr_pref += try_preference_from_file(
			EPREFIX "/etc/env.d/python/python2", curr_pref);
	curr_pref += try_preference_from_file(
			EPREFIX "/etc/env.d/python/python3", curr_pref);
}

#ifdef HAVE_READLINK

/**
 * Try to read a symlink target.
 *
 * @bufp points to the space in the buffer where the target shall be
 * written (first byte after the hyphen).
 *
 * @progname contains the program basename. May be NULL to disable
 * scriptbuf syntax enforcing.
 *
 * @path contains the symlink path.
 *
 * @max_len specifies the maximum value length. The buffer must have
 * at least one byte more for the null terminator.
 *
 * Returns 1 on success, 0 otherwise.
 */
static int try_symlink(char* bufp, const char* progname,
		const char* path, size_t max_len)
{
	size_t rd;

	errno = 0;
	/* 1 for the null terminator with max length */
	rd = readlink(path, bufp, max_len + 1);

	/* [max_len] could mean that the name is too long */
	if (rd > 0 && rd < max_len + 1)
	{
		if (progname)
		{
			bufp[rd] = path_sep;
			strcpy(&bufp[rd+1], progname);
		}
		else
			bufp[rd] = 0;
		return 1;
	}

	return 0;
}

#endif

#ifdef HAVE_READLINK

/**
 * Obtain symlink length. Assumes that symlinks don't change during
 * the process.
 *
 * @path contains the path to the symlink.
 *
 * Returns the symlink length or 0 if the file is not a symlink.
 */
static size_t get_symlink_length(const char* path)
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
 * Run the specified script using execvp(). Fallback to the 'env' tool
 * if the system does not support shebangs natively.
 *
 * @script specifies path to the script to execute.
 *
 * @argv specifies intended script argv. The passed array needs to start
 * one element earlier, so that argv[-1] assignment is valid.
 *
 * Does not return if execution succeeds. Returns otherwise.
 */
static void execute(char* script, char** argv)
{
	execv(script, argv);

	if (errno == ENOEXEC) /* mingw32? */
	{
		/* Preserve argv[0], and put the full path in argv[1]. */
		--argv;
		argv[0] = argv[1];
		argv[1] = script;

		execvp("env", argv);

		/* Restore old argv[0] pos. */
		argv[1] = argv[0];
	}
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
	char buf[BUFFER_SIZE];
	char scriptbuf[BUFFER_SIZE];
	char* bufpy;

	const char* slash;
	const char* script;
	int symlink_resolution = 0;

	const struct python_impl* i;
	int pref;

#ifndef NDEBUG
	/* initialize the buffers with some junk
	 * this helps catching missing null terminators */
	memset(buf, 'Z', sizeof(buf));
	memset(scriptbuf, 'Z', sizeof(buf));
#endif

	/* figure out basename from argv[0] */
	slash = strrchr(argv[0], path_sep);
	/* if we are called directly (via a shebang), script comes
	 * as argv[1] */
	if (!strcmp(slash ? &slash[1] : argv[0], "python-exec2-c"))
	{
		script = argv[1];
		++argv;

		if (!script || !script[0])
		{
			fprintf(stderr, "Usage: %s <script>\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	else /* otherwise, use argv[0] */
		script = argv[0];

	load_configuration();

	/* put the always-common part in */
	memcpy(scriptbuf, python_scriptroot, sizeof(python_scriptroot));
	bufpy = &scriptbuf[sizeof(python_scriptroot) - 1];

	while (1)
	{
		size_t len;
		char* fnpos;
		int j;

		if (!symlink_resolution)
			len = strlen(script);
#ifdef HAVE_READLINK
		else
		{
			size_t sym_len = get_symlink_length(buf);

			if (!sym_len)
			{
				if (errno != 0)
					fprintf(stderr, "%s: unable to stat symlink at %s: %s\n",
							script, buf, strerror(errno));
				else /* no more symlinks to try */
					fprintf(stderr, "%s: no supported Python implementation variant found!\n",
							script);
				break;
			}

			fnpos = strrchr(buf, path_sep);
			if (fnpos)
				len = &fnpos[1] - buf;
			else
				len = 0;

			len += sym_len;
		}
#endif

		/* length + null terminator */
		if (len + 1 > BUFFER_SIZE)
		{
			fprintf(stderr, "%s: program name longer than buffer size.\n",
					script);
			return 127;
		}

		if (!symlink_resolution)
			memcpy(buf, script, len + 1);
#ifdef HAVE_READLINK
		else
		{
			/**
			 * In order to support relative symlinks, preserve
			 * the current directory (but strip filename).
			 */
			if (!fnpos)
				fnpos = buf;
			else
				++fnpos;

			if (!try_symlink(fnpos, 0, buf, len))
			{
				fprintf(stderr, "%s: unable to read symlink at %s: %s.\n",
						script, buf,
						errno != 0 ? strerror(errno) : "target length changed");
				break;
			}

			/* Symlink is absolute, move the path. */
			if (*fnpos == path_sep && fnpos != buf)
				memmove(buf, fnpos, strlen(fnpos) + 1);
		}
#endif

		fnpos = strrchr(buf, path_sep);
		if (!fnpos)
			fnpos = buf;
		else
			++fnpos;

		/* scriptroot + '/' + EPYTHON + '/' + basename + '\0' */
		/* (but sizeof() gives [scriptroot + '/' + '\0']) */
		len = sizeof(python_scriptroot) + max_epython_len + strlen(fnpos) + 1;
		if (len >= BUFFER_SIZE)
		{
			fprintf(stderr, "%s: program name longer than buffer size.\n",
					fnpos);
			return 127;
		}

		/* Try j = 0..max-with-any-matches, then IMPL_DEFAULT */
		j = 0;
		while (1)
		{
			int found_any = 0;

			for (i = python_impls; i->name; ++i)
			{
				if (i->preference != j)
					continue;
				found_any = 1;

				set_scriptbuf(bufpy, i->name, fnpos);
				execute(scriptbuf, argv);
			}

			if (j == IMPL_DEFAULT)
				break;
			else if (!found_any)
				j = IMPL_DEFAULT;
			else
				++j;
		}

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
	return 127;
}
