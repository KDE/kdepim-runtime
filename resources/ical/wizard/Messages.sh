#! /usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
$XGETTEXT `find . \( -name \*.cpp -o -name \*.h -o -name \*.qml \)` -o  $podir/accountwizard_ical.pot
