AC_PREREQ([2.60])
AC_INIT([python-exec], [0.3])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH([python-impls],
	[AS_HELP_STRING([--with-python-impls=IMPLS],
		[The list of supported Python, most preferred last])])
AS_CASE(["$with_python_impls"], [yes|no|""],
[
	AC_MSG_ERROR([Python implementations must be specified using --with-python-impls])
])

# generate array of implementations
python_impls='0'
max_len=0
for i in $with_python_impls
do
	python_impls="\"$i\", $python_impls"
	AS_IF([test $max_len -lt ${#i}],
	[
		max_len=${#i}
	])
done

AC_DEFINE_UNQUOTED([PYTHON_IMPLS], [$python_impls],
	[Set to the list of supported Python implementations])
AC_SUBST([PYTHON_IMPLS], [$with_python_impls])
AC_DEFINE_UNQUOTED([MAX_EPYTHON_LEN], [$max_len],
	[Set to the maximum length of EPYTHON (implementation name)])

AC_ARG_WITH([eprefix],
	[AS_HELP_STRING([--with-eprefix=EPREFIX],
		[The Gentoo Prefix base path (EPREFIX)])])
AS_CASE(["$with_eprefix"],
	[yes], [AC_MSG_ERROR([--with-eprefix takes a value])],
	[no], [with_eprefix=])
AC_DEFINE_UNQUOTED([EPREFIX], ["$with_eprefix"],
	[Set to the Gentoo Prefix EPREFIX (if building for Prefix)])

AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2 subdir-objects parallel-tests])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_CHECK_FUNCS([readlink])
AM_CONDITIONAL([HAVE_READLINK], [test $ac_cv_func_readlink = yes])

AC_PROG_MKDIR_P
AC_PROG_SED

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
