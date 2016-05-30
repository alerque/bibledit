#!/bin/bash


# This script runs from within a Cygwin shell.
# It refreshes the bibledit library sources.
# It builds the bibledit library for Windows.
# It works in a temporal location outside of the source tree.


WINDOWSSOURCE=`dirname $0`
cd $WINDOWSSOURCE
BIBLEDITWINDOWS=/tmp/bibledit-windows
echo Synchronizing relevant source code to $BIBLEDITWINDOWS
mkdir -p $BIBLEDITWINDOWS
rsync --archive --delete ../lib $BIBLEDITWINDOWS/
rsync --archive --delete ../windows $BIBLEDITWINDOWS/


cd $BIBLEDITWINDOWS/windows


pushd webroot


rsync -av --delete ../../lib/ .
make distclean


# Limit the maximum number of parallel background tasks 
# to make the interface more responsive.
./configure --enable-client --enable-urlsettings --enable-paratext --with-parallel-tasks=1 --enable-windows --with-network-port=9876


make


mkdir -p /cygdrive/c/bibledit 
cp -r * /cygdrive/c/bibledit


popd
pushd /cygdrive/c/bibledit


find . -name "*.h" -delete
find . -name "*.cpp" -delete
find . -name "*.c" -delete
find . -name "*.o" -delete
rm *.m4
rm -r autom*cache
rm bibledit
rm compile
rm config.*
rm configure*
rm depcomp
rm dev
rm install-sh
rm missing
rm reconfigure
rm valgrind
find . -name ".deps" -exec rm -r "{}" \; > /dev/null 2>&1
find . -name ".dirstamp" -delete
rm Makefile*
rm stamp-h1
rm unittest.exe
rm libbibledit.a
rm -rf xcode
rm DEVELOP
rm INSTALL


cp /usr/bin/cyg*.dll .


popd
