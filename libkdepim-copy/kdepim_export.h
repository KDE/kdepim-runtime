/*
    This file is part of libkdepim.
    Copyright (c) 2006 Allen Winter <winter@kde.org>

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

#ifndef KDEPIM_EXPORT_H
#define KDEPIM_EXPORT_H

/* needed for KDE_EXPORT macros */
#include <kdepimmacros.h>

#if defined Q_OS_WIN
# include <kdepim_export_win.h>
#else /* UNIX */

/* export statements for unix */
#define AKONADI_EXPORT KDE_EXPORT
#define AKONADI_RESOURCES_EXPORT KDE_EXPORT
#define AKREGATOR_EXPORT KDE_EXPORT
#define KDEPIM_EXPORT KDE_EXPORT
#define KHOLIDAYS_EXPORT KDE_EXPORT
#define KLEO_EXPORT KDE_EXPORT
#define KODE_SCHEMA_EXPORT KDE_EXPORT
#define KSCHEMA_EXPORT KDE_EXPORT
#define KSIEVE_EXPORT KDE_EXPORT
#define KXMLCOMMON_EXPORT KDE_EXPORT
#define SYNDICATION_EXPORT KDE_EXPORT
#define KPGP_EXPORT KDE_EXPORT 
#define QGPGME_EXPORT KDE_EXPORT
#define KGROUPWAREDAV_EXPORT KDE_EXPORT
#define KGROUPWAREBASE_EXPORT KDE_EXPORT

#endif

#endif
