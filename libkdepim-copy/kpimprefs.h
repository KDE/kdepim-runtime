/*
  This file is part of libkdepim.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

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
#ifndef KPIMPREFS_H
#define KPIMPREFS_H

#include "kdepim_export.h"

#include <kconfigskeleton.h>
#include <kdatetime.h>

#include <QStringList>

class QString;

namespace KPIM {

class KDEPIM_EXPORT KPimPrefs : public KConfigSkeleton
{
  public:
    KPimPrefs( const QString &name = QString() );

    virtual ~KPimPrefs();

    /** Set preferences to default values */
    void usrSetDefaults();

    /** Read preferences from config file */
    void usrReadConfig();

    /** Write preferences to config file */
    void usrWriteConfig();

    /**
     * Get user's time zone.
     *
     * This will first look for whatever time zone is stored in KOrganizer's
     * configuration file.  If no time zone is found there, it uses the
     * local system time zone.
     *
     * @see Calendar
     */
    static KDateTime::Spec timeSpec();

  public:
    QStringList mCustomCategories;

  protected:
    virtual void setCategoryDefaults() {}

  public:
    static const QString categorySeparator;
};

}

#endif
