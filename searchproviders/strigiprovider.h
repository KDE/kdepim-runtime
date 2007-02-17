/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_STRIGIPROVIDER_H
#define AKONADI_STRIGIPROVIDER_H

#include <searchproviderbase.h>
#include <libakonadi/job.h>
#include <strigi/qtdbus/strigiclient.h>

namespace Akonadi {

class Session;
class Monitor;

/**
  Full text search provider using strigi.
*/
class StrigiProvider : public SearchProviderBase
{
  Q_OBJECT

  public:
    StrigiProvider( const QString &id );
    ~StrigiProvider();
    virtual QList<QByteArray> supportedMimeTypes() const { QList<QByteArray> l; l << "all/all"; return l; }

  protected slots:
    void itemChanged(const Akonadi::DataReference &ref);
    void itemRemoved(const Akonadi::DataReference &ref);
    void itemReceived( KJob *job );

  private:
    Monitor *mMonitor;
    Session *mQueue;
    StrigiClient mStrigi;
};

}

#endif
