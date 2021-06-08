AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-buffer-size=128 \
	--with-python-impls='pypy pypy3 python2.7 python3.6 python3.7 python3.8 python3.9 python3.10'

bin_PROGRAMS = python-exec2c
scriptroot_SCRIPTS = python-exec2

check_DATA = test/data/etc/python-exec/python-exec.conf
check_PROGRAMS = test/bin/print-bufsiz test/bin/python-exec2c
check_SCRIPTS = test/bin/python-exec2

python_exec2c_SOURCES = src/python-exec.c
python_exec2c_CPPFLAGS = \
	-DEXEEXT=\"${EXEEXT}\" \
	-DPYTHON_SCRIPTROOT=\"${scriptrootdir}\" \
	-DSYSCONFDIR=\"${sysconfdir}\" \
	-DNDEBUG

test_bin_python_exec2c_SOURCES = src/python-exec.c
test_bin_python_exec2c_CPPFLAGS = \
	-DEXEEXT=\"${EXEEXT}\" \
	-DPYTHON_SCRIPTROOT=\"test/data\" \
	-DSYSCONFDIR=\"test/data/etc\" \
	-UNDEBUG

test_bin_print_bufsiz_SOURCES = src/print-bufsiz.c

EXTRA_DIST = src/python-exec.in config/python-exec.conf.example
CLEANFILES = python-exec2 \
	test/bin/* \
	test/data/etc/python-exec/*.conf \
	test/data/* test/data/etc/python-exec/* test/data/*/*

python-exec2: src/python-exec.in config.status
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|${bindir}|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|${scriptrootdir}|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

test/data/etc/python-exec/python-exec.conf: tests/gen-config.sh config.status
	$(MKDIR_P) test/data/etc/python-exec
	rm -f $@ $@.tmp
	sh $(srcdir)/tests/gen-config.sh $(PYTHON_IMPLS) > $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

test/bin/python-exec2: src/python-exec.in config.status
	$(MKDIR_P) test/bin
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|./test/bin|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|test/data|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

LOG_COMPILER = sh $(srcdir)/tests/runner.sh "$(PYTHON_IMPLS)"

TESTS = tests/epython-test tests/eselect-test tests/order-test \
	tests/pythonwrap-test tests/pythonwrap-__file__-test \
	tests/pythonwrap-argv0-test tests/long-path-test \
	tests/disable-test tests/epython-disable-test \
	tests/order-disable-test tests/python-argv0-test \
	tests/wrapper-copy-test tests/pythonwrap-copy-test \
	tests/config-script-override-test
XFAIL_TESTS = tests/long-path-test tests/disable-test

SYMLINK_BASED_TESTS = \
	tests/symlink-test tests/abs-symlink-test \
	tests/deep-symlink-test tests/pythonwrap-symlink-test \
	tests/pythonwrap-abs-symlink-test tests/pythonwrap-deep-symlink-test \
	tests/long-symlink-test tests/deep-long-symlink-test \
	tests/c-symlink-test tests/c-symlink-path-test

if HAVE_READLINK
TESTS += \
	$(SYMLINK_BASED_TESTS)
XFAIL_TESTS += \
	tests/long-symlink-test tests/deep-long-symlink-test
else
EXTRA_DIST += $(SYMLINK_BASED_TESTS)
endif

EXTRA_DIST += tests/runner.sh tests/gen-config.sh $(TESTS)

EXTRA_DIST += COPYING NEWS README
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
