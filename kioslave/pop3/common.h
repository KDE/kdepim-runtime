/*  This file is part of the KDE project
    Copyright (C) 2008 Jarosław Staniek <staniek@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _KIOSLAVE_COMMON_H
#define _KIOSLAVE_COMMON_H

#include <stdio.h>
#include <QFile>
#include <QDir>

extern "C" {
#include <sasl/sasl.h>
}

inline bool initSASL()
{
#ifdef Q_OS_WIN32  //krazy:exclude=cpp
    QByteArray libInstallPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("lib") + QLatin1String("sasl2"))));
    QByteArray configPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("config") + QLatin1String("sasl2"))));
    if (sasl_set_path(SASL_PATH_TYPE_PLUGIN, libInstallPath.data()) != SASL_OK ||
            sasl_set_path(SASL_PATH_TYPE_CONFIG, configPath.data()) != SASL_OK) {
        fprintf(stderr, "SASL path initialization failed!\n");
        return false;
    }
#endif

    if (sasl_client_init(NULL) != SASL_OK) {
        fprintf(stderr, "SASL library initialization failed!\n");
        return false;
    }
    return true;
}

#endif
