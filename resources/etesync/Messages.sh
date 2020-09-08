#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT *.cpp *.h -o $podir/akonadi_etesync_resource.pot
