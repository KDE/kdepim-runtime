#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT libmaildir/*.cpp *.cpp -o $podir/akonadi_maildir_resource.pot
