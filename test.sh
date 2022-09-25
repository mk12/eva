#!/bin/bash

set -eufo pipefail

usage() {
	cat <<EOS
Usage: $0 [clean] [FILE.scm ...]
EOS
}

cd "$(dirname "$0")"

if [[ $# -eq 1 ]]; then
	case $1 in
		-h|--help|help)
			usage
			exit
			;;
		clean)
			rm -f test.log compare.sh
			rm -rf test/out
			exit
			;;
	esac
fi

mkdir -p test/{src,ref,out}

log=test.log
: > "$log"

cmp_sh=compare.sh

if [[ $# -eq 0 ]]; then
	files=()
	while read -r f; do
		files+=("$f")
	done < <(find test/src -name "*.scm")
else
	files=("$@")
fi

# Echo to the log file.
log() {
	echo "*** test.sh: $1" >> "$log"
}

# Echo to standard output and the log file.
echolog() {
	echo "$1"
	log "$1"
}

# Compare the reference file against actual output.
compare() {
	if cmp -s "$1" "$2"; then
		echo -n .
	else
		echo -n F
		log "Difference detected on $1"
		diff "$1" "$2" >> "$log"

		if [[ ${#failures[@]} -eq 0 ]]; then
			echo -e "#!/bin/bash\nvimdiff $1 $2" > "$cmp_sh"
			chmod +x "$cmp_sh"
		fi
		failures+=("$1")
		return 1
	fi
}

# Print message to say test is being skipped.
skip_test() {
	echo -n s
	log "Skipping test $1 (no ref found)"
}

# Count the number of arguments.
count() {
	echo $#
}

# Count the total number of tests to be run.
total=${#files[@]}

# Adjust total based on presence of files.
for f in "${files[@]}"; do
	base=${f##*/}
	base=${base%.scm}
	src=test/src/$base.scm
	ref=test/ref/$base.ref
	repl_ref=test/ref/${base}_repl.ref

	if [[ ! -f $src ]]; then
		echolog "Warning: File $src does not exist"
		((total--))
	elif [[ -f $ref && -f $repl_ref ]]; then
		((total++))
	fi
done

# Run the regression tests.
exitstatus=0
reg_counter=0
regression() {
	failures=()

	for f in "${files[@]}"; do
		base=${f##*/}
		base=${base%.scm}
		src=test/src/$base.scm
		ref=test/ref/$base.ref
		out=test/out/$base.out
		repl_ref=test/ref/${base}_repl.ref
		repl_out=test/out/${base}_repl.out

		if [[ ! -f "$src" ]]; then
			continue
		fi

		skip=y
		if [[ -f "$ref" ]]; then
			skip=n
			bin/eva -n "$src" > "$out" 2>&1
			compare "$ref" "$out"
			((reg_counter++))
		fi
		if [[ -f "$repl_ref" ]]; then
			skip=n
			bin/eva -n < "$src" > "$repl_out" 2>&1
			compare "$repl_ref" "$repl_out"
			((reg_counter++))
		fi

		if [[ $skip == "y" ]]; then
			skip_test "$src"
		fi
	done
}

# Pluralize the word "test".
ntests() {
	if [[ $1 -eq 1 ]]; then
		echo test
	else
		echo tests
	fi
}

# Print a summary of the test results.
finish() {
	if [[ ${#failures[@]} -eq 0 ]]; then
		echo " ok"
	else
		echo " FAIL"
	fi

	all="All "
	if [[ $reg_counter -ne $total ]]; then
		all=
		skipped=$((total - reg_counter))
		echolog "Note: Skipped $skipped/$total $(ntests "$total")"
	fi

	if [[ ${#failures[@]} -eq 0 ]]; then
		echolog "$all$reg_counter $(ntests "$reg_counter") passed"
	else
		num=${#failures[@]}
		echolog "$num/$total $(ntests "$total") failed"
		echolog "Failures:"
		for f in "${failures[@]}"; do
			echolog "- $f"
		done
		exitstatus=1
	fi

	if [[ $exitstatus -eq 1 ]]; then
		echo "Run $cmp_sh to see the first diff"
	fi
	exit $exitstatus
}

echo "Running regression tests"
trap finish SIGINT
regression
finish
