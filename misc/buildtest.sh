#!/bin/bash

echo -n "Cleaning Stuff now"

rm -rf _inst > /dev/null
echo -n "."
rm -rf libopensync-0.?? > /dev/null
echo -n "."
rm -f libopensync-0.??.tar.gz > /dev/null
echo -n "."
echo ""

TOP_SRCDIR=$(pwd)

echo -n "Making sure build environment is sane"
./autogen.sh > /dev/null
echo -n "."
make clean > /dev/null || exit 1
echo -n "."

echo "Making tarball"
make dist > /dev/null || exit 1

mkdir _inst > /dev/null
rm -rf libopensync-0.??

echo "Unpacking tarball"
tar zxvf libopensync-0.??.tar.gz > /dev/null || exit 1

cd libopensync-0.??

echo -n "Making OpenSync"
./autogen.sh --prefix=$TOP_SRCDIR/_inst > /dev/null || exit 1
echo -n "."
make install > /dev/null || exit 1
echo "."

cd docs/example-plugin

echo -n "Making example plugin"
export PKG_CONFIG_PATH=$TOP_SRCDIR/_inst/lib
./autogen.sh --prefix=$TOP_SRCDIR/_inst > /dev/null || exit 1
echo -n "."
make install > /dev/null || exit 1
echo "."

cd $TOP_SRCDIR/libopensync-0.?? || exit 1

echo "Running tests now."
make check || exit 1

cd $TOP_SRCDIR

rm -rf _inst
rm -rf libopensync-0.??

echo "Everyting went fine!"
