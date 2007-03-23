/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_NNTPRESOURCE_H
#define AKONADI_NNTPRESOURCE_H

#include <resourcebase.h>

#include <kio/job.h>

#include <QtDBus/QDBusMessage>

class NntpResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    NntpResource( const QString &id );
    ~NntpResource();

  public Q_SLOTS:
    virtual bool requestItemDelivery( const Akonadi::DataReference &ref, int type, const QDBusMessage &msg );
    virtual void configure();

  protected:
    void retrieveCollections();
    void synchronizeCollection( const Akonadi::Collection &col );

    /**
      Returns the base url used for all KIO operations, containing
      protocol, hostname and port.
    */
    QString baseUrl() const;

  private slots:
    void listGroups( KIO::Job* job, const KIO::UDSEntryList &list );
    void listGroupsResult( KJob* job );

    void listGroup( KIO::Job* job, const KIO::UDSEntryList &list );
    void listGroupResult( KJob* job );

    void fetchArticleResult( KJob* job );

  private:
    Akonadi::Collection::List remoteCollections;
    Akonadi::Collection mCurrentCollection;

    // FIXME: this breaks with multiple concurrent requests
    Akonadi::DataReference mCurrentRef;
    QDBusMessage mCurrentMessage;

    QString mConfig;
};

#endif
