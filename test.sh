#!/bin/bash

cd "$(dirname "$0")"

# Usage message.
if [[ $1 == "-h" || $1 == "--help" || $1 == "help" ]]; then
	echo "usage: test.sh [clean] [FILE.scm ...]"
	exit 0
fi

# Clean generated files.
if [[ $1 == "clean" ]]; then
	rm -f test.log compare.sh test/out/*.out
	exit 0
fi

# Create directories
mkdir -p test/{src,ref,out}

# Clear the log file.
LOG=test.log
> $LOG

# Name of the compare script.
CMPSH=compare.sh

# Get the regression test files.
if [[ $# -eq 0 ]]; then
	files=test/src/*.scm
else
	files=$@
fi

# Echo to the log file.
log() {
	echo "*** test.sh: $1" >> $LOG
}

# Echo to standard output and the log file.
echolog() {
	echo "$1"
	log "$1"
}

# Compare the reference file against actual output.
failures=
compare() {
	if cmp -s "$1" "$2"; then
		echo -n .
	else
		echo -n F
		log "Difference detected on $1"
		diff "$1" "$2" >> $LOG

		if [[ -z $failures ]]; then
			echo -e "#!/bin/bash\nvimdiff $1 $2" > $CMPSH
			chmod +x $CMPSH
		fi
		failures+=" $1"
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
total=$(count $files)

# Adjust total based on presence of files.
for f in $files; do
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
	failures=

	for f in $files; do
		base=${f##*/}
		base=${base%.scm}
		src=test/src/$base.scm
		ref=test/ref/$base.ref
		out=test/out/$base.out
		repl_ref=test/ref/${base}_repl.ref
		repl_out=test/out/${base}_repl.out

		if [[ ! -f $src ]]; then
			continue
		fi

		skip=y
		if [[ -f $ref ]]; then
			skip=n
			bin/eva -n $src > $out 2>&1
			compare $ref $out
			((reg_counter++))
		fi
		if [[ -f $repl_ref ]]; then
			skip=n
			bin/eva -n < $src > $repl_out 2>&1
			compare $repl_ref $repl_out
			((reg_counter++))
		fi

		if [[ $skip == "y" ]]; then
			skip_test $src
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
	if [[ -z $failures ]]; then
		echo " ok"
	else
		echo " FAIL"
	fi

	all="All "
	if [[ $reg_counter -ne $total ]]; then
		all=
		skipped=$((total - reg_counter))
		echolog "Note: Skipped $skipped/$total $(ntests $total)"
	fi

	if [[ -z $failures ]]; then
		echolog "$all$reg_counter $(ntests $reg_counter) passed"
	else
		num=$(count $failures)
		echolog "$num/$total $(ntests $total) failed"
		echolog "Failures:"
		for f in $failures; do
			echolog "- $f"
		done
		exitstatus=1
	fi

	if [[ $exitstatus -eq 1 ]]; then
		echo "Run $CMPSH to see the first diff"
	fi
	exit $exitstatus
}

echo "Running regression tests"
trap finish SIGINT
regression
finish
