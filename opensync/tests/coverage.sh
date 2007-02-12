#!/bin/bash

echo "Checking Code Coverage:"
mv ../opensync/.libs/*.bb ../opensync &> /dev/null
mv ../opensync/.libs/*.bbg ../opensync &> /dev/null
mv ../opensync/.libs/*.da ../opensync &> /dev/null

mv ../osengine/.libs/*.bb ../osengine &> /dev/null
mv ../osengine/.libs/*.bbg ../osengine &> /dev/null
mv ../osengine/.libs/*.da ../osengine &> /dev/null

lcov -d ../opensync -d ../osengine -d . -q -c -o coverage/app.info

genhtml -o coverage coverage/app.info &> /dev/null

#lcov -d ../opensync -q -z &> /dev/null
#lcov -d ../osengine -q -z &> /dev/null
#lcov -d . -q -z &> /dev/null

echo -n "Code Coverage is: "
grep " %</td>" coverage/index.html | sed -e "s/^[^>]*>//g" -e "s/<[^>]*>//g"
echo -n ""
exit 0
