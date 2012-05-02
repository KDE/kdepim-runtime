#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.kcfg -o -name \*.ui` >> rc.cpp || exit 11
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/akonadi_google_resource.pot
rm -f rc.cpp
