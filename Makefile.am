AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-python-impls='python3.1 python3.2 python2.6 python2.7'

bin_PROGRAMS = python-exec-c
bin_SCRIPTS = python-exec

check_PROGRAMS = tests/print-bufsiz
check_SCRIPTS = tests/python-exec

python_exec_c_SOURCES = src/python-exec-c.c
tests_print_bufsiz_SOURCES = src/print-bufsiz.c

EXTRA_DIST = src/python-exec.in
CLEANFILES = python-exec tests/python-exec tests/*.tmp*
python-exec: src/python-exec.in
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|${bindir}|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

tests/python-exec: python-exec
	$(MKDIR_P) tests
	rm -f $@ $@.tmp
	@SED@ -e "s|${bindir}|.|" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

LOG_COMPILER = sh $(srcdir)/tests/runner.sh "$(PYTHON_IMPLS)"

TESTS = tests/epython-test tests/eselect-test tests/order-test \
	tests/pythonwrap-test tests/pythonwrap-__file__-test \
	tests/long-path-test
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
