#!/bin/sh

bar=`$CMD /dev/null 2>&1`

if [ "$?" != "0" ]; then
	echo "$bar"
	exit $?
fi

echo "$bar" | grep '       0 B \[================= N/A ================\] 0 B/s       (ela 00m 00s)'

exit $?
