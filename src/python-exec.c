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
const char sys_path_sep = ':';

enum python_impl_preference
{
	IMPL_DEFAULT = -1,
	IMPL_DISABLED = -2,
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
 * Find basename component in path.
 *
 * @path The path.
 *
 * Returns pointer to the basename component.
 */
static const char* find_basename(const char* path)
{
	const char* fnpos = strrchr(path, path_sep);
	if (fnpos)
		return &fnpos[1];
	else
		return path;
}

#ifdef HAVE_READLINK
/**
 * Resolve symlinks up to the final symlink to python-exec2.
 *
 * @buf Output buffer to store the resulting path in.
 * Must be of BUFFER_SIZE length.
 *
 * @path Path to the initial executable (symlink).
 *
 * Returns 1 on success, 0 on failure.
 */
int resolve_symlinks(char* outbuf, const char* path)
{
	char second_buf[BUFFER_SIZE];
	int bufno = 0;
	size_t orig_path_len;

	int need_path_lookup = 0;
	const char* sys_path;
	const char* path_it;

#ifndef NDEBUG
	/* initialize the buffer with some junk
	 * this helps catching missing null terminators */
	memset(second_buf, 'Z', sizeof(second_buf));
#endif

	/* We use two buffer interchangeably to resolve symlinks until we
	 * reach EINVAL (non-symlink). We consider two cases:
	 *
	 * a. final executable is named python-exec2, then we use the final
	 *    symlink to it,
	 *
	 * b. final executable is named otherwise, then we assume
	 *    python-exec was copied rather than symlinked and use that.
	 *
	 * For each symlink resolution step:
	 *
	 * 1. buf contains target(i-1),
	 *
	 * 2. buf2 contains target(i),
	 *
	 * 3. buf = readlink(target(i)).
	 *
	 * 3a. if readlink() succeeds, target(i-1) is irrelevant and buf
	 *     gets target(i+1), we exchange buffers and continue,
	 *
	 * 3b. if readlink() fails, buf is not overwritten and target(i-1)
	 *     is still there.
	 */

	/* Now, this kinda sucks but if we're symlinked directly to the C
	 * wrapper, argv[0] may not contain any path. We need to do PATH
	 * lookup in this case.
	 */

	orig_path_len = strlen(path);

	/* if executable has any path, we're good */
	if (strchr(path, path_sep))
	{
		/* path + '\0' */
		if (orig_path_len + 1 > BUFFER_SIZE)
		{
			fprintf(stderr, "%s: path longer than buffer size.\n",
					path);
			return 0;
		}

		strcpy(outbuf, path);
	}
	else
	{
		need_path_lookup = 1;

		sys_path = getenv("PATH");
		/* mimic exec*p() behavior */
		if (!sys_path)
			sys_path = "";

		path_it = 0;
	}

	/* pre-set to null in case someone tried to run python-exec2 */
	second_buf[0] = '\0';

	while (1)
	{
		/* buffer containing the current symlink to resolve */
		char* curr_path = bufno ? second_buf : outbuf;
		/* buffer for result (containing previous path) */
		char* res_path = bufno ? outbuf : second_buf;

		if (need_path_lookup)
		{
			const char* sep_pos;
			size_t dir_len;

			if (!path_it)
				path_it = sys_path;
			else if (path_it[0] == '\0')
			{
				fprintf(stderr, "%s: unable to find executable in PATH.\n",
						path);
				return 0;
			}
			else /* it's on separator then */
				++path_it;

			/* find next separator or end-of-string */
			sep_pos = strchr(path_it, sys_path_sep);
			if (!sep_pos)
				sep_pos = path_it + strlen(path_it);
			dir_len = sep_pos - path_it;

			/* dir '/' executable '\0' */
			if (dir_len + orig_path_len + 2 > BUFFER_SIZE)
			{
				fprintf(stderr, "%s: system PATH component longer than buffer size.\n",
						path);
				return 0;
			}

			/* empty component works as current directory */
			if (dir_len == 0)
				outbuf[0] = '\0';
			else
			{
				strncpy(outbuf, path_it, dir_len);
				outbuf[dir_len] = path_sep;
				outbuf[dir_len+1] = '\0';
			}
			strcat(outbuf, path);

			/* set path_it for the next iteration */
			path_it = sep_pos;

			/* verify if it's executable, so we don't wind up in dead
			 * end resolving the wrong path */
			if (access(outbuf, X_OK) == -1)
			{
				/* EACCES -- not executable
				 * ENOENT -- dangling symlink
				 * ENOTDIR -- invalid symlink target
				 */
				if (errno == EACCES || errno == ENOENT || errno == ENOTDIR)
					continue;
				else
				{
					fprintf(stderr, "%s: unable to test executable %s: %s.\n",
							path, curr_path, strerror(errno));
					return 0;
				}
			}
		}

		/* find basename offset in curr_path */
		const char* curr_fnpos = find_basename(curr_path);
		size_t fnoff = curr_fnpos - curr_path;
		/* set to basename position of curr_path in res_path,
		 * so that we can copy directory path straight if we get
		 * a relative symlink */
		char* fnpos = &res_path[fnoff];
		/* free space in the buffer */
		ssize_t max_length = BUFFER_SIZE - fnoff;

		/* Note: we pass max_length even though we need one byte
		 * for null terminator -- this way, we can check if path was
		 * not truncated by readlink() */
		ssize_t ret = readlink(curr_path, fnpos, max_length);

		if (ret == -1)
		{
			/* if we're doing PATH lookup, skip failing entry and try
			 * next PATH */
			if (need_path_lookup && errno == ENOENT)
				continue;
			else if (errno == EINVAL)
			{
				const char* res;

				/* ok, curr_path was the last symlink;
				 * now let's see if it's python-exec2 */
				if (!strcmp(curr_fnpos, "python-exec2")
						|| !strcmp(curr_fnpos, "python-exec2c" EXEEXT))
				{
					/* let's see if we succeeded at least once */
					if (!res_path[0])
					{
						fprintf(stderr, "%s: python-exec2 is a wrapper, "
								"it must not be run directly.\n",
								curr_path);
						return 0;
					}

					/* curr_path is python-exec, so let's use last path
					 * that is still in res_path (since readlink()
					 * failed) */
					res = res_path;
				}
				else
				{
					/* curr_path is not python-exec, so it's probably
					 * a copy of python-exec, so let's use it. */
					res = curr_path;
				}

				/* copy result to outbuf if it happened to be
				 * in the other buffer */
				if (outbuf != res)
					strcpy(outbuf, res);

				break;
			}
			else
			{
				fprintf(stderr, "%s: unable to resolve symlink %s: %s.\n",
						path, curr_path, strerror(errno));
				return 0;
			}
		}
		else if (ret == max_length)
		{
			fprintf(stderr, "%s: symlink %s target longer than buffer size.\n",
					path, curr_path);
			return 0;
		}

		/* disable PATH lookup if readlink() succeeded */
		need_path_lookup = 0;

		/* add a null terminator */
		fnpos[ret] = '\0';

		/* now, if we got an absolute path, then move it to front;
		 * otherwise, copy base path from curr_path */
		if (*fnpos == '/')
			memmove(res_path, fnpos, ret+1);
		else
			memcpy(res_path, curr_path, fnoff);

		/* exchange buffers */
		bufno = !bufno;
	}

	return 1;
}
#endif

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
 * Try to read full implementation preference from specified
 * configuration file.
 *
 * @path Path to the configuration file.
 *
 * @pref Minimal requested preference value.
 *
 * Returns 1 if preferences were read from the file, 0 otherwise.
 * In the latter case, legacy files should be read instead.
 */
static int try_preferences_from_config(const char* path, int pref)
{
	char buf[BUFFER_SIZE];
	int continuation = 0;

	FILE* f = fopen(path, "r");

	if (f)
	{
		while (fgets(buf, sizeof(buf), f))
		{
			int impl_ret;
			const char* impl = buf;
			int impl_pref = pref;
			size_t len = strlen(buf);

			/* Strip the trailing newline, and decrease length */
			if (buf[len-1] == '\n')
				buf[--len] = '\0';
			/* No newline is allowed on EOF */
			else if (!feof(f))
			{
				/* Ignore the following read as a continuation */
				continuation = 1;
				continue;
			}
			else if (continuation)
			{
				/* This is a continuation of a very long line, ignore */
				continuation = 0;
				continue;
			}

			/* Ignore empty lines and comments */
			if (len == 0 || buf[0] == '#')
				continue;

			/* Handle disabling implementations */
			if (buf[0] == '-')
			{
				++impl;
				impl_pref = IMPL_DISABLED;
			}

			impl_ret = set_impl_preference(impl, impl_pref);
			/* == pref intentional here to avoid ++pref on disabled */
			if (impl_ret == pref)
				++pref;
			else if (impl_ret == IMPL_DEFAULT)
				fprintf(stderr, "python-exec: Invalid impl in %s: %s\n",
						path, buf);
		}

		if (ferror(f))
			fprintf(stderr, "python-exec: Error reading %s: %s\n",
					path, strerror(errno));

		fclose(f);
		return 1;
	}
	else if (errno != ENOENT)
		fprintf(stderr, "python-exec: Unable to open %s: %s\n",
			path, strerror(errno));

	return 0;
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
 *
 * @scriptname The script basename (used for config overrides).
 */
static void load_configuration(const char* scriptname)
{
	/**
	 * The implementation check order:
	 * 1) environment variable EPYTHON (local choice),
	 * 2a) python-exec.conf or...
	 * 2b1) eselect-python main Python interpreter,
	 * 2b2) eselect-python Python 2 & Python 3 choices,
	 * 3) any of the supported implementations.
	 *
	 * For 2b2), the order is basically irrelevant since whichever
	 * is preferred will be tried in 2b1) anyway.
	 *
	 * 3) uses the eclass-defined order.
	 */
	int curr_pref = 0;
	const char* epython;

	char configbuf[BUFFER_SIZE];
	char* configbuf_fn_pos;

	epython = getenv("EPYTHON");
	if (epython)
	{
		if (set_impl_preference(epython, curr_pref) == curr_pref)
			++curr_pref;
		else
			fprintf(stderr, "python-exec: EPYTHON value invalid (%s).\n",
					epython);
	}

	/* common prefix */
	/* two sizeof()s == 2 null terminators, we need only one so -1 */
	if (sizeof(SYSCONFDIR "/python-exec/") + sizeof(".conf") - 1
			+ strlen(scriptname) > sizeof(configbuf))
	{
		fprintf(stderr, "python-exec: configuration path longer than the buffer ("
				SYSCONFDIR "/python-exec/%s.conf), overrides will be ignored.\n",
				scriptname);
	}
	else
	{
		strcpy(configbuf, SYSCONFDIR "/python-exec/");
		strcat(configbuf, scriptname);
		strcat(configbuf, ".conf");

		if (try_preferences_from_config(configbuf, curr_pref))
			return;
	}

	if (!try_preferences_from_config(
				SYSCONFDIR "/python-exec/python-exec.conf", curr_pref))
	{
		curr_pref += try_preference_from_file(
				SYSCONFDIR "/env.d/python/config", curr_pref);
		curr_pref += try_preference_from_file(
				SYSCONFDIR "/env.d/python/python2", curr_pref);
		curr_pref += try_preference_from_file(
				SYSCONFDIR "/env.d/python/python3", curr_pref);
	}
}

/**
 * Run the specified script using execvp().
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

	/* warn about other errors but try hard to run something */
	if (errno != ENOENT)
		fprintf(stderr, "python-exec: Unable to execute %s: %s.\n",
				script, strerror(errno));
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
	char scriptbuf[BUFFER_SIZE];
#ifdef HAVE_READLINK
	char fnbuf[BUFFER_SIZE];
#endif
	char* bufpy;

	const char* slash;
	const char* script;

	const struct python_impl* i;
	int pref;

	size_t len;
	const char* fnpos;
	int j;


#ifndef NDEBUG
	/* initialize the buffers with some junk
	 * this helps catching missing null terminators */
	memset(scriptbuf, 'Z', sizeof(scriptbuf));
#	ifdef HAVE_READLINK
	memset(fnbuf, 'Z', sizeof(fnbuf));
#	endif
#endif

	/* figure out basename from argv[0] */
	slash = strrchr(argv[0], path_sep);
	/* if we are called directly (via a shebang), script comes
	 * as argv[1] */
	if (!strcmp(slash ? &slash[1] : argv[0], "python-exec2c" EXEEXT))
	{
		script = argv[1];

		if (!script || !script[0])
		{
			fprintf(stderr, "Usage: %s <script>\n", argv[0]);
			return EXIT_FAILURE;
		}
		else if (!strcmp(script, "--help") || !strcmp(script, "-h"))
		{
			fprintf(stderr, "Usage: %s <script>\n"
"\n"
"python-exec is a wrapper to run Python scripts in an environment\n"
"supporting parallel install of multiple Python implementations.\n"
"For more information, please see the included README file.\n"
"\n"
"Additional options:\n"
"  --help, -h         print this help message\n"
"  --version, -V      print the package name and version\n"
					"", argv[0]);
			return EXIT_SUCCESS;
		}
		else if (!strcmp(script, "--list-implementations")
				|| !strcmp(script, "-l"))
		{
			for (i = python_impls; i->name; ++i)
			{
				fprintf(stderr, "%s\n", i->name);
			}
			return EXIT_SUCCESS;
		}
		else if (!strcmp(script, "--version") || !strcmp(script, "-V"))
		{
			fprintf(stderr, PACKAGE_STRING "\n");
			return EXIT_SUCCESS;
		}

		++argv;
	}
	else /* otherwise, use argv[0] */
		script = argv[0];

	/* put the always-common part in */
	memcpy(scriptbuf, python_scriptroot, sizeof(python_scriptroot));
	bufpy = &scriptbuf[sizeof(python_scriptroot) - 1];

	/* pass the full wrapped script path as argv[0], because:
	 * a. Linux will do it anyway for interpreted scripts, so this
	 *    guarantees consistent behavior across platforms,
	 * b. some programs do realpath(argv[0]) in order to find their
	 *    executable -- it ends up badly if they find python-exec
	 *    instead. This is especially the case when wrapping python
	 *    itself and scripts do os.path.realpath(sys.executable).
	 */
	argv[0] = scriptbuf;

#ifdef HAVE_READLINK
	/* perform symlink resolution */
	if (!resolve_symlinks(fnbuf, script))
		return 127;

	fnpos = find_basename(fnbuf);
#else
	fnpos = find_basename(script);
#endif

	/* scriptroot + '/' + EPYTHON + '/' + basename + '\0' */
	/* (but sizeof() gives [scriptroot + '/' + '\0']) */
	len = sizeof(python_scriptroot) + max_epython_len + strlen(fnpos) + 1;
	if (len >= BUFFER_SIZE)
	{
		fprintf(stderr, "%s: program name longer than buffer size.\n",
				fnpos);
		return 127;
	}

	load_configuration(fnpos);

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

			strcpy(bufpy, i->name);
			strcat(bufpy, "/");
			strcat(bufpy, fnpos);
			execute(scriptbuf, argv);
		}

		if (j == IMPL_DEFAULT)
			break;
		else if (!found_any)
			j = IMPL_DEFAULT;
		else
			++j;
	}

	/* If no execvp() succeeded, that means we either don't have
	 * a single supported implementation here or something is seriously
	 * broken.
	 */
	return 127;
}
