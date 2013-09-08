#!/bin/sh
$EXTRACTRC ui/*.ui >> rc.cpp
$XGETTEXT *.cpp ispdb/*.cpp  -o $podir/accountwizard.pot
