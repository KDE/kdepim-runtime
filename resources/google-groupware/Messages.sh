#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT *.cpp -o $podir/akonadi_google_resource.pot
