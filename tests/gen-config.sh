#!/bin/sh
# write python-exec.conf for tests

main() {
	no_impls=${#}

	if [ ${#} -ge 4 ]; then
		# disable the most preferred interpreter
		echo "-${4}"
	fi

	# then the least preferred
	if [ ${#} -gt 3 ]; then
		echo "${1}"
	fi
}

main "${@}"
