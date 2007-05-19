#!/bin/bash

echo -n "Checking Code Coverage of unit tests"
DATE=$( date +%Y%m%d%H%M%S )

if [ -d .svn ]; then
	REV=`LANG=C svn info | grep Revision | cut -c 11-`
	echo " for SVN Revision: $REV"
	TITLE="OpenSync_SVN"$REV"_"$DATE
else
	echo ":"
	TITLE="OpenSync_"$DATE
fi	

if ! [ -d coverage/html ]; then
	mkdir -p coverage/html
fi

echo $TITLE
lcov -t "$TITLE" -b ../ -d ../ -q -c -o coverage/$TITLE.info
genhtml --legend -t "$TITLE" -o coverage/html/$TITLE coverage/$TITLE.info &> /dev/null

# XXX: reset counters ( lcov -z seems to be outdated... tries to dlete .da files instead of .gcda ) 
find ../ -name "*.gcda" | xargs rm

echo -n "Code Coverage is: "
grep " %</td>" coverage/html/$TITLE/index.html | sed -e "s/^[^>]*>//g" -e "s/<[^>]*>//g"
echo -n ""
echo -e "\nTroubleshooting:\n If the Code Coverage number is quite low (less then 51%):"
echo -e "\t-Did you run any unit tests before $0?"
echo -e "\t-Did you build with enable_profiling=1?"
echo -e "\t-Run ALL available unit tests!"
echo -e "\t-Check if testcases in unit test are disabled!"
echo -e "\t-Fix unit tests and their test cases!"
echo -e "\t-Write new and more test cases!"
exit 0
