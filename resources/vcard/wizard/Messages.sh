#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: CC0-1.0
$XGETTEXT `find . \( -name \*.cpp -o -name \*.h -o -name \*.qml \)` -o  $podir/accountwizard_vcard.pot
