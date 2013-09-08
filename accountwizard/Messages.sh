#!/bin/sh
$EXTRACTRC ui/*.ui >> rc.cpp
$XGETTEXT *.cpp *.h -o $podir/accountwizard.pot
