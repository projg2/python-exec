AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-python-impls='python3.1 python3.2 python2.6 python2.7'

bin_PROGRAMS = python-exec-c
bin_SCRIPTS = python-exec

python_exec_c_SOURCES = src/python-exec-c.c

EXTRA_DIST = src/python-exec.in
CLEANFILES = python-exec
python-exec: src/python-exec.in
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|${bindir}|" ${srcdir}/src/$@.in > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

EXTRA_DIST += NEWS
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
