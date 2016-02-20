AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-buffer-size=128 \
	--with-python-impls='pypy python2.7 python3.3 python3.4 python3.5'

bin_PROGRAMS = python-exec2c
scriptroot_SCRIPTS = python-exec2

check_DATA = tests/etc/python-exec/python-exec.conf
check_PROGRAMS = tests/print-bufsiz tests/python-exec2c
check_SCRIPTS = tests/python-exec2

python_exec2c_SOURCES = src/python-exec.c
python_exec2c_CPPFLAGS = \
	-DEXEEXT=\"${EXEEXT}\" \
	-DPYTHON_SCRIPTROOT=\"${scriptrootdir}\" \
	-DSYSCONFDIR=\"${sysconfdir}\" \
	-DNDEBUG

tests_python_exec2c_SOURCES = src/python-exec.c
# TODO: create dedicated sysconfdir for tests
tests_python_exec2c_CPPFLAGS = \
	-DEXEEXT=\"${EXEEXT}\" \
	-DPYTHON_SCRIPTROOT=\"tests\" \
	-DSYSCONFDIR=\"tests/etc\" \
	-UNDEBUG

tests_print_bufsiz_SOURCES = src/print-bufsiz.c

EXTRA_DIST = src/python-exec.in config/python-exec.conf.example
CLEANFILES = python-exec2 \
	tests/python-exec2 tests/python-exec2c \
	tests/etc/python-exec/*.conf \
	tests/*.tmp* tests/*/*.tmp* \
	tests/python tests/*/python

python-exec2: src/python-exec.in config.status
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|${bindir}|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|${scriptrootdir}|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

tests/etc/python-exec/python-exec.conf: tests/gen-config.sh config.status
	$(MKDIR_P) tests/etc/python-exec
	rm -f $@ $@.tmp
	sh $(srcdir)/tests/gen-config.sh $(PYTHON_IMPLS) > $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

tests/python-exec2: src/python-exec.in config.status
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
	tests/pythonwrap-argv0-test \
	tests/long-path-test tests/c-symlink-test \
	tests/disable-test tests/epython-disable-test \
	tests/order-disable-test tests/python-argv0-test \
	tests/wrapper-copy-test tests/pythonwrap-copy-test \
	tests/config-script-override-test
XFAIL_TESTS = tests/long-path-test tests/disable-test

if HAVE_READLINK
TESTS += \
	tests/symlink-test tests/abs-symlink-test \
	tests/deep-symlink-test tests/pythonwrap-symlink-test \
	tests/pythonwrap-abs-symlink-test tests/pythonwrap-deep-symlink-test \
	tests/long-symlink-test tests/deep-long-symlink-test
XFAIL_TESTS += \
	tests/long-symlink-test tests/deep-long-symlink-test
endif

EXTRA_DIST += tests/runner.sh tests/gen-config.sh $(TESTS)

EXTRA_DIST += COPYING NEWS README
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
