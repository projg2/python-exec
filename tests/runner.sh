#!/bin/sh

if [ ${#} -ne 2 ]; then
	echo "Usage: ${0} <python-impls> <test-script>"
	exit 1
fi

# automake codes
SKIP=77
ERROR=99

# common metadata
PYTHON_IMPLS=${1}
TEST=${2}
TEST_NAME=${2##*/}
TEST_TMP=tests/${TEST_NAME}.tmp

# helper functions
write_impl() {
	echo "${2}" > "${TEST_TMP}-${1}"
	chmod -w,+x "${TEST_TMP}-${1}"
}

do_exit() {
	trap - EXIT
	exit "${@}"
}

do_test() {
	set +e

	"${@}"
	ret=${?}
	echo "Test result: ${ret}" >&2
	do_exit ${ret}
}

get_eselected() {
	set +e

	set -- eselect python show "${@}"
	ret=$("${@}")
	echo "${ret}"

	[ -n "${ret}" ] || ret='(none)'
	echo "${*} -> ${ret}" >&2

	set -e
}

# catch all failures
trap 'exit 99' EXIT
set -e

rm -f "${TEST_TMP}"*
ln -s python-exec "${TEST_TMP}"
. "${TEST}"
