#!/bin/sh

testDir=$(dirname $0)
cd "$testDir"

fail=""

if which md5; then
	export MD5=$(which md5)
else
	export MD5=$(which md5sum)
fi

export CMD="./pp"

echo "Running tests"

for test in *.test.sh; do

	echo -n " - $test ... "

	cd ".."

	output=$(sh "$testDir/$test" 2>&1)
	retCode=$?

	if [ "$retCode" = "0" ]; then
		echo "ok"
	else
		echo "failed. Error code $retCode, output:"
		echo "$output" | sed -e 's/^/   	/'

		fail="$fail $test"
	fi

	cd "$testDir"
done

if [ -z "$fail" ]; then
	echo "done"
	exit 0
else
	echo "FAILED:$fail"
	exit 1
fi
