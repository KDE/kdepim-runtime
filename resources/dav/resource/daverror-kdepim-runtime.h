/*
    SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDAV/DavError>

/**
 * @deprecated Use KDAV::DavError::errorText() instead
 */
QString translateErrorString(const KDAV::Error &error);
