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

#ifndef KJOTSAKONADIPAGE_H
#define KJOTSAKONADIPAGE_H

#include <QString>
// #include <QLatin1String>

// #include <Qt>

class QString;
class QLatin1String;
class QIODevice;
class QDomDocument;

// namespace Akonadi
// {
// class Item;
// }

#include <akonadi/item.h>
#include <KUrl>
#include <QDomDocument>

#include "kjotspage.h"

using namespace Akonadi;

class KJotsAkonadiPage
{
  enum Changes {
    ChangedPayload,
    ChangedDisplayName
  };

public:
  KJotsAkonadiPage();
  KJotsAkonadiPage( Item item );
  KJotsAkonadiPage( KJotsPage page );
//     KJotsAkonadiPage(QDomDocument doc);
//     KJotsAkonadiPage(const QString &remoteId);
  ~KJotsAkonadiPage();

  void setRemoteId( const QString &remoteId );

  bool isValid();

  QString remoteId();

  static QLatin1String mimeType() {
    return QLatin1String( "application/x-vnd.kde-app-kjots-page" );
  }
//     static KJotsAkonadiPage fromIODevice ( QIODevice *dev );
//     static KJotsAkonadiPage fromRemoteId ( const QString &remoteId );

  Item writeOut();
  int changes();

  Item getItem();

private:
//   void setDomDocument(QDomDocument doc);

  QString m_title;
  QString m_content;
  Item m_item;
  QString m_remoteId;
  int m_changes;
  QDomDocument m_domDocument;
  QString m_rootDataPath;
  KJotsPage m_page;
};

#endif
