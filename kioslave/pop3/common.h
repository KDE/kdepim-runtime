/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Jarosław Staniek <staniek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KIOSLAVE_COMMON_H
#define _KIOSLAVE_COMMON_H

#include <QDir>
#include <QFile>
#include <stdio.h>

extern "C" {
#include <sasl/sasl.h>
}

inline bool initSASL()
{
#ifdef Q_OS_WIN32 // krazy:exclude=cpp
#if 0
    QByteArray libInstallPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("lib") + QLatin1String("sasl2"))));
    QByteArray configPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("config") + QLatin1String("sasl2"))));
    if (sasl_set_path(SASL_PATH_TYPE_PLUGIN, libInstallPath.data()) != SASL_OK
        || sasl_set_path(SASL_PATH_TYPE_CONFIG, configPath.data()) != SASL_OK) {
        fprintf(stderr, "SASL path initialization failed!\n");
        return false;
    }
#endif
#endif

    if (sasl_client_init(nullptr) != SASL_OK) {
        fprintf(stderr, "SASL library initialization failed!\n");
        return false;
    }
    return true;
}

#endif
