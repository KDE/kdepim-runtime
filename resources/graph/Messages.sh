#! /usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
$EXTRACTRC *.kcfg >> rc.cpp
$XGETTEXT `find . -name \*.cpp` -o $podir/akonadi_graph_resource.pot
