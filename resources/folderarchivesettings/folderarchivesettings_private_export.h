/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "folderarchivesettings_export.h"

/* Classes which are exported only for unit tests */
#ifdef BUILD_TESTING
#ifndef FOLDERARCHIVESETTINGS_TESTS_EXPORT
#define FOLDERARCHIVESETTINGS_TESTS_EXPORT FOLDERARCHIVESETTINGS_EXPORT
#endif
#else /* not compiling tests */
#define FOLDERARCHIVESETTINGS_TESTS_EXPORT
#endif
