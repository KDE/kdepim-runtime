#! /usr/bin/env bash
$EXTRACTRC *.ui *.kcfg >> rc.cpp
$XGETTEXT *.cpp ../common/*.cpp -o $podir/akonadi_davgroupware_resource.pot
