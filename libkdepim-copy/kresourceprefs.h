/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KRESOURCEPREFS_H
#define KRESOURCEPREFS_H

#include <kconfigskeleton.h>
#include <kdepimmacros.h>

class QString;

/**
  This is a base class for all KPrefs objects, where multiple instances want
  to work on the same config file.
  By calling addGroupPrefix( "foobar" ), 'foobar' as a prefix is added to the
  group names in the configuration file.
  The prefix should be an unique identifier to avoid name clashes and the method
  has to be called before readConfig(), otherwise the wrong entries are read.
 */
class KDE_EXPORT KResourcePrefs : public KConfigSkeleton
{
  public:
    KResourcePrefs( const QString &name = QString::null );

    /**
      Adds a prefix to all groups of this prefs object.
     */
    void addGroupPrefix( const QString &prefix );
};

#endif
