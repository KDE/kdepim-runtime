#! /usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
$EXTRACTRC `find . -name \*.kcfg -o -name \*.ui` >> rc.cpp || exit 11
$XGETTEXT `find . \( ! -path "./kdepim-runtime/*" \) -a \( -name "*.cpp" -o -name "*.h" \)` -o $podir/akonadi_kalarm_resource.pot
rm -f rc.cpp
