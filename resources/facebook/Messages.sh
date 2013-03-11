#! /usr/bin/env bash
$EXTRACTRC `find . -name '*.ui' -o -name '*.kcfg'` >> rc.cpp || exit 11
$XGETTEXT *.cpp -o $podir/akonadi_facebook_resource.pot
rm -f rc.cpp
