AC_PREREQ([2.60])
AC_INIT([python-exec], [2.4.10])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH([python-impls],
	[AS_HELP_STRING([--with-python-impls=IMPLS],
		[The list of supported Python, most preferred last])])

case $with_python_impls in
	yes|no|"")
		AC_MSG_ERROR([Python implementations must be specified using --with-python-impls])
		;;
esac

# generate array of implementations
python_impls='{ 0, 0 }'
max_len=0
for i in $with_python_impls
do
	python_impls="{ \"$i\", IMPL_DEFAULT }, $python_impls"
	if test $max_len -lt ${#i}; then
		max_len=${#i}
	fi
done

AC_DEFINE_UNQUOTED([PYTHON_IMPLS], [$python_impls],
	[Set to the list of supported Python implementations])
AC_DEFINE_UNQUOTED([MAX_EPYTHON_LEN], [$max_len],
	[Set to the maximum length of EPYTHON (implementation name)])

AC_ARG_WITH([buffer-size],
	[AS_HELP_STRING([--with-buffer-size=SIZE],
		[Initial static buffer size (default: BUFSIZ)])])
case $with_buffer_size in
	yes) AC_MSG_ERROR([--with-buffer-size takes a numeric value]);;
	no) AC_MSG_ERROR([--without-buffer-size is meaningless]);;
	"") ;;
	*)
		AC_DEFINE_UNQUOTED([BUFFER_SIZE], [$with_buffer_size],
			[Set to static buffer size for path])
		;;
esac

AC_ARG_WITH([python-scriptroot],
	[AS_HELP_STRING([--with-python-scriptroot=PATH],
		[Root directory for wrapped python scripts (default: \${exec_prefix}/lib/python-exec)])],,
	[
		with_python_scriptroot=\${exec_prefix}/lib/python-exec
	])
AC_SUBST([scriptrootdir], [$with_python_scriptroot])

default_fallback_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
AC_ARG_WITH([fallback-path],
	[AS_HELP_STRING([--with-fallback-path=PATH], [Fallback PATH setting)])],,
	[with_fallback_path="$default_fallback_path"])

case $with_fallback_path in
	yes) with_fallback_path="$default_fallback_path" ;;
	no) AC_MSG_ERROR([--without-fallback-path is not allowed]) ;;
esac

AC_DEFINE_UNQUOTED([FALLBACK_PATH], ["$with_fallback_path"], [Set to fallback PATH])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-bzip2 subdir-objects parallel-tests])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CHECK_FUNCS([readlink])
AM_CONDITIONAL([HAVE_READLINK], [test $ac_cv_func_readlink = yes])

AC_PROG_MKDIR_P
AC_PROG_SED

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_LINKS([
	test/conftest.py:test/conftest.py
	test/test_core.py:test/test_core.py
	test/test_select.py:test/test_select.py
])
AC_OUTPUT
