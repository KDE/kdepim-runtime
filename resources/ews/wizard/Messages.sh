#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/accountwizard_ews.pot
touch $podir/accountwizard_ews.pot
$XGETTEXT -kqsTr *.es -j -o $podir/accountwizard_ews.pot
