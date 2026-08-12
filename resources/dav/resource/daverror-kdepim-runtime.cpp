/*
    SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "daverror-kdepim-runtime.h"

#include <KLocalizedString>

using namespace KDAV;

QString translateErrorString(const KDAV::Error &error)
{
    return error.errorText();
}
