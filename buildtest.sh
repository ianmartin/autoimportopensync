#!/bin/bash

echo -n "Cleaning Stuff now"

rm -rf _inst > /dev/null
echo -n "."
rm -rf opensync-0.?? > /dev/null
echo -n "."
rm -f opensync-0.??.tar.gz > /dev/null
echo -n "."
echo ""

TOP_SRCDIR=$(pwd)

echo -n "Making sure build environment is sane"
./autogen.sh &> /dev/null
echo -n "."
make clean &> /dev/null || exit 1
echo -n "."

cd plugins/file-sync

./autogen.sh &> /dev/null
echo -n "."
make clean &> /dev/null || exit 1
echo -n "."
echo ""

cd $TOP_SRCDIR

echo "Making tarball"
make dist > /dev/null || exit 1

mkdir _inst > /dev/null
rm -rf opensync-0.??

echo "Unpacking tarball"
tar zxvf opensync-0.??.tar.gz > /dev/null || exit 1

cd opensync-0.??

echo -n "Making OpenSync"
./autogen.sh --prefix=$TOP_SRCDIR/_inst &> /dev/null || exit 1
echo -n "."
make install > /dev/null || exit 1
echo "."

cd plugins/file-sync

echo -n "Making file-sync plugin"
export PKG_CONFIG_PATH=$TOP_SRCDIR/_inst/lib
./autogen.sh --prefix=$TOP_SRCDIR/_inst --enable-error-tests=yes &> /dev/null || exit 1
echo -n "."
make install > /dev/null || exit 1
echo "."

cd $TOP_SRCDIR/opensync* || exit 1

echo "Running tests now."
make check || exit 1

cd $TOP_SRCDIR

rm -rf _inst
rm -rf opensync-0.??

echo "Everyting went fine!"
