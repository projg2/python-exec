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
TEST_DIR=tests
TEST_NAME=${2##*/}
TEST_TMP=${TEST_NAME}.tmp

# helper functions
write_impl() {
	mkdir -p "${TEST_DIR}/${1}" && \
	printf '%b' "${2}" > "${TEST_DIR}/${1}/${TEST_TMP}${3}" && \
	chmod -w,+x "${TEST_DIR}/${1}/${TEST_TMP}${3}"
}

do_sym() {
	ln -s "${1}" "${TEST_DIR}/${2}"
}

do_exit() {
	trap - EXIT
	exit "${@}"
}

do_test() {
	if [ ${#} -eq 2 ]; then
		set -- "${1}" "${TEST_DIR}/${2}"
	else
		set -- "${TEST_DIR}/${1}"
	fi

	set +e +x
	echo "Test command: ${@}" >&2

	"${@}"
	ret=${?}
	echo "Test result: ${ret}" >&2
	do_exit ${ret}
}

is_preferred() {
	set +e +x

	while read l; do
		case "${l}" in
			'#'*|-*)
				;;
			"${1}")
				set -e
				return 0
				;;
		esac
	done <tests/etc/python-exec/python-exec.conf

	set -e -x
	return 1
}

is_disabled() {
	set +e +x

	while read l; do
		case "${l}" in
			'#'*)
				;;
			-"${1}")
				set -e
				return 0
				;;
		esac
	done <tests/etc/python-exec/python-exec.conf

	set -e -x
	return 1
}

# catch all failures
trap 'exit 99' EXIT
set -e -x

rm -f "${TEST_DIR}/${TEST_TMP}"* "${TEST_DIR}"/*/"${TEST_TMP}"*
ln -s python-exec2 "${TEST_DIR}/${TEST_TMP}"
. "${TEST}"
