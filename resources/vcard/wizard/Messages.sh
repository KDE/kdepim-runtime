#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/accountwizard_vcard.pot
$XGETTEXT -kqsTr *.es.cmake -j -o $podir/accountwizard_vcard.pot
