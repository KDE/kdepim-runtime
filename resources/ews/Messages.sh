#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT `find . -name \*.cpp` -o $podir/akonadi_ews_resource.pot
