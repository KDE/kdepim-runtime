#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui` >> rc.cpp || exit 11
$XGETTEXT *.cpp *.h -o $podir/akonadi_singlefile_resource.pot
rm -f rc.cpp
