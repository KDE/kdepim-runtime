#! /usr/bin/env bash
$EXTRACTRC *.kcfg >> rc.cpp
$XGETTEXT *.cpp -o $podir/akonadi_facebook_resource.pot
rm -f rc.cpp
