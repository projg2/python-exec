AC_PREREQ([2.60])
AC_INIT([python-exec], [2.2])
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
python_impls='0'
max_len=0
for i in $with_python_impls
do
	python_impls="\"$i\", $python_impls"
	if test $max_len -lt ${#i}; then
		max_len=${#i}
	fi
done

AC_DEFINE_UNQUOTED([PYTHON_IMPLS], [$python_impls],
	[Set to the list of supported Python implementations])
AC_SUBST([PYTHON_IMPLS], [$with_python_impls])
AC_DEFINE_UNQUOTED([MAX_EPYTHON_LEN], [$max_len],
	[Set to the maximum length of EPYTHON (implementation name)])

AC_ARG_WITH([eprefix],
	[AS_HELP_STRING([--with-eprefix=EPREFIX],
		[The Gentoo Prefix base path (EPREFIX)])])
case $with_eprefix in
	yes) AC_MSG_ERROR([--with-eprefix takes a value]);;
	no) with_eprefix=;;
esac
AC_DEFINE_UNQUOTED([EPREFIX], ["$with_eprefix"],
	[Set to the Gentoo Prefix EPREFIX (if building for Prefix)])

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

AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2 subdir-objects parallel-tests])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CHECK_FUNCS([readlink])
AM_CONDITIONAL([HAVE_READLINK], [test $ac_cv_func_readlink = yes])

AC_PROG_MKDIR_P
AC_PROG_SED

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
