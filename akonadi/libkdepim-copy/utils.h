/**
 * utils.h
 *
 * Copyright (C) 2007 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef KDEPIM_UTILS_H
#define KDEPIM_UTILS_H

#include "kdepim_export.h"
#include <QString>

namespace KPIM {

class KDEPIM_EXPORT Utils
{
  public:
    static QString rot13( const QString &s );

#ifdef Q_WS_WIN
    /**
     * Sets @a pids to a list of processes having name @a processName.
     */
    static void getProcessesIdForName( const QString& processName, QList<int>& pids );

    /**
     * @return true if one or more processes (other than the current process) exist for name @a processName.
     */
    static bool otherProcessesExist( const QString& processName );

    /**
     * Terminates or kills all processes with name @a processName.
     * First, SIGTERM is sent to a process, then if that fails, we try with SIGKILL.
     * @return true on successful termination of all processes or false if at least 
     *         one process failed to terminate.
     */
    static bool killProcesses( const QString& processName );

    /**
     * Activates window for first found process with executable @a executableName 
     * (without path and .exe extension)
     */
    static void activateWindowForProcess( const QString& executableName );
#endif
};

}

#endif

