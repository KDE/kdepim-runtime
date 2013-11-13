#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/accountwizard_maildir.pot
$XGETTEXT -kqsTr *.es -j -o $podir/accountwizard_maildir.pot
