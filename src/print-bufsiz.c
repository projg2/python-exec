/* python-exec -- a Gentoo tool to choose the correct Python script
 * variant for currently selected Python implementation.
 * (c) 2012 Michał Górny
 * Licensed under the terms of the 2-clause BSD license.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdio.h>

#ifndef BUFFER_SIZE
#	define BUFFER_SIZE BUFSIZ
#endif

int main()
{
	long unsigned buf_size = BUFFER_SIZE;
	printf("%lu\n", buf_size);
	return 0;
}
