/* python-wrapper-r1
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

const char* const python_impls[] = { PYTHON_IMPLS };

int main(int argc, char* argv[])
{
	const char* const* i;
	char buf[BUFSIZ];
	char* bufp = buf;

	size_t len = strlen(argv[0]);

	const char* epython = getenv("EPYTHON");

	if (len + 32 >= BUFSIZ)
		bufp = malloc(len + 32);
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
