#! /bin/sh

$XGETTEXT `find . -name '*.cpp' | grep -v '/test/' | grep -v '/util' ` -o $podir/akonadi_nepomuk_feeder.pot
