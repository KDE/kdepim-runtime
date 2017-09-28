#!/bin/bash

dobuild=0
doprep=0
dotest=0
doinstall=0

srcdir=$(pwd)

while [ $# -gt 0 ]; do
    case "$1" in
        --build|-b)
            dobuild=1
            shift
        ;;

        --prep|-p)
            doprep=1
            shift
        ;;

        --test|-t)
            dotest=1
            shift
        ;;

        --install|-i)
            doinstall=1
            shift
        ;;

    esac
done

if [ ${dobuild} -eq 0 -a ${doprep} -eq 0 -a ${dotest} -eq 0 -a ${doinstall} -eq 0 ]; then
    dobuild=1
    doprep=1
    dotest=1
    doinstall=1
fi

version_major=`grep -E "^set\s*\(Libkolab_VERSION_MAJOR [0-9]+\)" CMakeLists.txt | sed -r -e 's/^set\s*\(Libkolab_VERSION_MAJOR ([0-9]+)\)/\1/g'`
version_minor=`grep -E "^set\s*\(Libkolab_VERSION_MINOR [0-9]+\)" CMakeLists.txt | sed -r -e 's/^set\s*\(Libkolab_VERSION_MINOR ([0-9]+)\)/\1/g'`
version_patch=`grep -E "^set\s*\(Libkolab_VERSION_PATCH [0-9]+\)" CMakeLists.txt | sed -r -e 's/^set\s*\(Libkolab_VERSION_PATCH ([0-9]+)\)/\1/g'`

if [ -z "${version_patch}" ]; then
    version="${version_major}.${version_minor}"
else
    version="${version_major}.${version_minor}.${version_patch}"
fi

# Rebuilds the entire foo in one go. One shot, one kill.
rm -rf build/
mkdir -p build
cd build
if [ ${doprep} -eq 1 ]; then
    cmake \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DLIB_INSTALL_DIR=/usr/lib64 \
        -DINCLUDE_INSTALL_DIR=/usr/include \
        -DUSE_LIBCALENDARING=ON \
        -DPHP_BINDINGS=ON \
        -DPHP_INSTALL_DIR=/usr/lib64/php/modules \
        -DPYTHON_BINDINGS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        ..
fi

if [ ${dobuild} -eq 1 ]; then
    make
fi

if [ ${dotest} -eq 1 ]; then
    # Execute some tests?
    pushd tests
    unset DISPLAY
    ./benchmarktest
    ./calendaringtest
    ./formattest
    ./freebusytest
    ./icalendartest
    ./kcalconversiontest
    ./upgradetest
    popd
fi

if [ ${doinstall} -eq 1 ]; then
    make install DESTDIR=${TMPDIR:-/tmp}
fi

cd ..

rm -rf libkolab-${version}.tar.gz
git archive --prefix=libkolab-${version}/ HEAD | gzip -c > libkolab-${version}.tar.gz

rm -rf `rpm --eval='%{_sourcedir}'`/libkolab-${version}.tar.gz
cp libkolab-${version}.tar.gz `rpm --eval='%{_sourcedir}'`


