#!/bin/bash

cd "$(dirname "$0")"

if [[ $1 == "clean" ]]; then
	rm -f test.log compare.sh test/out/*.out
	exit 0
fi

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

# Start a new step in the test suite.
step_counter=1
step() {
	echolog "[ $step_counter ] $1"
	((step_counter++))

	if [[ $# -eq 1 ]]; then
		return
	fi

	${@:2} >> $LOG 2>&1

	st=$?
	if [[ $st -ne 0 ]]; then
		echolog "Command failed with exit status $st"
		echo "See output in $LOG"
		exit 1
	fi
}

# Compare the reference file against actual output.
failures=
compare() {
	if cmp -s "$1" "$2"; then
		echo -n .
	else
		echo -n F
		log "Difference detected on $3"
		diff "$1" "$2" >> $LOG

		if [[ -z $failures ]]; then
			echo -e "#!/bin/bash\nvimdiff $1 $2" > $CMPSH
			chmod +x $CMPSH
		fi
		failures+=" $3"
		return 1
	fi
}

# Count the number of arguments.
count() {
	echo $#
}

total=$(count $files)

# Run the regression tests.
exitstatus=0
reg_counter=0
regression() {
	failures=

	for src in $files; do
		base=${src##*/}
		ref=test/ref/$base.ref
		out=test/out/$base.out

		if [[ $base == repl_* ]]; then
			bin/eva < $src > $out 2>&1
		else
			bin/eva $src > $out 2>&1
		fi
		compare $ref $out $src

		((reg_counter++))
	done
}

ntests() {
	if [[ $1 -eq 1 ]]; then
		echo test
	else
		echo tests
	fi
}

finish() {
	all="All "
	if [[ $reg_counter -ne $total ]]; then
		all=
		skipped=$((total - reg_counter))
		echolog "Note: Skipped $skipped/$total $(ntests $total)"
	fi

	if [[ -z $failures ]]; then
		echo " ok"
		echolog "$all$reg_counter $(ntests $reg_counter) passed"
	else
		echo " FAIL"
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

step "Initializing tup" tup init
step "Building unit tests" tup
step "Running unit tests" bin/test
step "Running regression tests"
trap finish SIGINT
regression
finish
