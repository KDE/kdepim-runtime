/*
    This file is part of libkdepim.

    Copyright (c) 2005 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#ifndef KPIM_EMBEDDEDURLPAGE_H
#define KPIM_EMBEDDEDURLPAGE_H

#include <qwidget.h>
#include <kdepimmacros.h>
#include <kurl.h>

namespace KParts { class ReadOnlyPart; }

namespace KPIM {

class KDE_EXPORT EmbeddedURLPage : public QWidget
{
    Q_OBJECT
  public:
    EmbeddedURLPage( const QString &url, const QString &mimetype,
                     QWidget *parent, const char *name = 0 );

  public slots:
    void loadContents();
  signals:
    void openURL( const KURL &url );
  private:
    void initGUI( const QString &url, const QString &mimetype );

    QString mUri;
    QString mMimeType;
    KParts::ReadOnlyPart* mPart;
};

}

#endif
