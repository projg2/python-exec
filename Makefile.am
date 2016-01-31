AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-python-impls='pypy python2.7 python3.3 python3.4 python3.5'

scriptroot_PROGRAMS = python-exec2-c
scriptroot_SCRIPTS = python-exec2

check_PROGRAMS = tests/print-bufsiz tests/python-exec2-c
check_SCRIPTS = tests/python-exec

python_exec2_c_SOURCES = src/python-exec-c.c
python_exec2_c_CPPFLAGS = -DPYTHON_SCRIPTROOT=\"${scriptrootdir}\" -DNDEBUG

tests_python_exec2_c_SOURCES = tests/python-exec-c.c
tests_python_exec2_c_CPPFLAGS = -DPYTHON_SCRIPTROOT=\"tests\" -UNDEBUG

tests_print_bufsiz_SOURCES = src/print-bufsiz.c

BUILT_SOURCES = tests/python-exec-c.c
EXTRA_DIST = src/python-exec.in
CLEANFILES = python-exec2 \
	tests/python-exec tests/python-exec2-c \
	tests/python-exec-c.c tests/*.tmp* tests/*/*.tmp*

python-exec2: src/python-exec.in
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|${scriptrootdir}|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|${scriptrootdir}|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

tests/python-exec-c.c: src/python-exec-c.c
	$(MKDIR_P) tests
	cp $< $@

tests/python-exec: src/python-exec.in
	$(MKDIR_P) tests
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|./tests|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|tests|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

LOG_COMPILER = sh $(srcdir)/tests/runner.sh "$(PYTHON_IMPLS)"

TESTS = tests/epython-test tests/eselect-test tests/order-test \
	tests/pythonwrap-test tests/pythonwrap-__file__-test \
	tests/long-path-test tests/c-symlink-test
XFAIL_TESTS = tests/long-path-test

if HAVE_READLINK
TESTS += \
	tests/symlink-test tests/abs-symlink-test tests/symlink-pref-test \
	tests/deep-symlink-test tests/pythonwrap-symlink-test \
	tests/pythonwrap-abs-symlink-test tests/pythonwrap-deep-symlink-test \
	tests/long-symlink-test tests/deep-long-symlink-test
XFAIL_TESTS += \
	tests/long-symlink-test tests/deep-long-symlink-test
endif

EXTRA_DIST += tests/runner.sh $(TESTS)

EXTRA_DIST += NEWS
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
