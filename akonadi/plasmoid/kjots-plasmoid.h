/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


// Here we avoid loading the header multiple times
#ifndef KJOTSPLASMOID_H
#define KJOTSPLASMOID_H

#include <Plasma/PopupApplet>

class KJotsWidget;

// Define our plasma Applet
class KJotsPlasmoid : public Plasma::PopupApplet
{
  Q_OBJECT
public:
  // Basic Create/Destroy
  KJotsPlasmoid( QObject *parent, const QVariantList &args );
  virtual ~KJotsPlasmoid();

  virtual QWidget *widget();

private:
  KJotsWidget *w;
};

// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_APPLET( kjots, KJotsPlasmoid )

#endif
