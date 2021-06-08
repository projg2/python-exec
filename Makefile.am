AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-buffer-size=128 \
	--with-python-impls='pypy pypy3 python2.7 python3.6 python3.7 python3.8 python3.9 python3.10'

bin_PROGRAMS = python-exec2c
scriptroot_SCRIPTS = python-exec2

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

test/bin/python-exec2: src/python-exec.in config.status
	$(MKDIR_P) test/bin
	rm -f $@ $@.tmp
	@SED@ -e "s|[@]bindir@|./test/bin|" \
		-e "s|[@]PYTHON_SCRIPTROOT@|test/data|" \
		-e "s|[@]exeext@|${EXEEXT}|g" $< > $@.tmp
	chmod a-w,a+x $@.tmp
	mv $@.tmp $@

PYTEST_FILES = \
	test/conftest.py \
	test/test_core.py \
	test/test_select.py
EXTRA_DIST += $(PYTEST_FILES)

check-local: $(PYTEST_FILES)
	pytest -vv -ra -l test

EXTRA_DIST += COPYING NEWS README
NEWS: configure.ac Makefile.am
	git for-each-ref refs/tags --sort '-*committerdate' \
		--format '# %(tag) (%(*committerdate:short))%0a%(contents:body)' \
		> NEWS
