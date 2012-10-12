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

int main(int argc, char* argv[])
{
	const char* const* i;
	char buf[BUFSIZ];
	char* bufp = buf;

	size_t len = strlen(argv[0]);

	const char* epython = getenv("EPYTHON");

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

	if (epython)
	{
		if (strlen(epython) <= 30)
		{
			strcpy(&bufp[len+1], epython);
			execvp(bufp, argv);
		}
		else
			fprintf(stderr, "%s: EPYTHON value invalid (too long).\n",
					argv[0]);
	}

	{
		const char* const config_path = "/etc/env.d/python/config";
		FILE* f = fopen(config_path, "r");

		if (f)
		{
			size_t rd = fread(&bufp[len+1], 1, 30, f);

			if (rd > 0 && feof(f))
			{
				bufp[len+rd+1] = 0;
				if (bufp[len+rd] == '\n')
					bufp[len+rd] = 0;

				fclose(f);
				execvp(bufp, argv);
			}
			else
				fclose(f);
		}
		else
			fprintf(stderr, "%s: unable to open %s: %s.\n",
					argv[0], config_path, strerror(errno));
	}

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
