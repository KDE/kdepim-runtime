#! /bin/sh
$EXTRACTRC `find . -name '*.kcfg'` >> rc.cpp || exit 11
$XGETTEXT *.cpp -o $podir/akonadi_newmailnotifier_agent.pot
rm -f rc.cpp
