/*
   This file is part of the KDE project
   Copyright (C) 2003 Tobias Koenig <tokoe@kde.org>

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

#ifndef  ABOUTDATAEXTENSION_H
#define  ABOUTDATAEXTENSION_H

#include <qobject.h>

class KAboutData;

namespace KParts
{

  class ReadOnlyPart;

  /**
    Provides a way to export the about data.
   */
  class AboutDataExtension : public QObject
  {
    Q_OBJECT

    public:
      /**
        Constucts a AboutDataExtension.

        @param about    @ref KAboutData structur.
        @param parent   The parent widget.
        @param name     The name of the class.
       */
      AboutDataExtension( KAboutData *about, KParts::ReadOnlyPart *parent,
                          const char* name );
      ~AboutDataExtension();

      /**
        Retrieve a pointer to the about data.
        May be 0 if 0 was passed in the constructor
       **/
      KAboutData* aboutData() const { return mAboutData; };

    private:
      KAboutData* mAboutData;

      class Private;
      Private *d;
  };
}

#endif
