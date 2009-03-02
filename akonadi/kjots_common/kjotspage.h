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

#ifndef KJOTSPAGE_H
#define KJOTSPAGE_H

#include <QString>
#include <QDomDocument>
// #include <QLatin1String>

// #include <Qt>

class QString;
class QLatin1String;
class QIODevice;

class KJotsPage
{
public:
  KJotsPage();
  KJotsPage( const QString &remoteId );
  KJotsPage( QDomDocument doc );
  ~KJotsPage();

  void setTitle( const QString &title );
  void setContent( const QString &content );
  void setRemoteId( const QString &remoteId );

  bool isValid();
  bool save();

  QString title();
  QString content();
  QString remoteId();

  static QLatin1String mimeType() {
    return QLatin1String( "application/x-vnd.kde-app-kjots-page" );
  }
  static KJotsPage fromIODevice( QIODevice *dev );

private:
  void setDomDocument( QDomDocument doc );
  QString m_title;
  QString m_content;
  QString m_remoteId;
  QString m_rootDataPath;
  QDomDocument m_domDocument;
};

#endif
