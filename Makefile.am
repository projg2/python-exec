AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-python-impls='python3.1 python3.2 python2.6 python2.7'

bin_PROGRAMS = python-exec

python_exec_SOURCES = src/python-exec.c

EXTRA_DIST = NEWS
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
