#!/bin/bash

TOP_SRCDIR=$(pwd)

make clean || exit 1

cd plugins/file-sync

make clean || exit 1

cd $TOP_SRCDIR

make dist || exit 1

mkdir _inst > /dev/null
rm -rf opensync-0.??

tar zxvf opensync* || exit 1

cd opensync*

./configure --prefix=$TOP_SRCDIR/_inst || exit 1
make install || exit 1

cd plugins/file-sync

./configure --prefix=$TOP_SRCDIR/_inst || exit 1
make install || exit 1

cd $TOP_SRCDIR/opensync* || exit 1

make check || exit 1

cd $TOP_SRCDIR

rm -rf _inst
rm -rf opensync-0.??
