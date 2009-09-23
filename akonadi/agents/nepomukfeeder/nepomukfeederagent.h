/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>
                  2009 Volker Krause <vkrause@kde.org>

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

#ifndef NEPOMUKFEEDERAGENT_H
#define NEPOMUKFEEDERAGENT_H

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/mimetypechecker.h>

#include "resource.h"

#include <QStringList>

namespace Akonadi
{
  class Item;
}

class KJob;

/** Shared base class for all Nepomuk feeders. */
class NepomukFeederAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    NepomukFeederAgent(const QString& id);
    ~NepomukFeederAgent();

    /** Remove all references to the given item from Nepomuk. */
    static void removeItemFromNepomuk( const Akonadi::Item &item );

    /** Adds tags to @p resource based on the given string list. */
    static void tagsFromCategories( NepomukFast::Resource &resource, const QStringList &categories );

    /** Add a supported mimetype. */
    void addSupportedMimeType( const QString &mimeType );

    /** Reimplement to do the actual work. */
    virtual void updateItem( const Akonadi::Item &item ) = 0;

  public slots:
    /** Trigger a complete update of all items. */
    void updateAll();

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

  private:
    void processNextCollection();
    void selfTest();

  private slots:
    void collectionsReceived( const Akonadi::Collection::List &collections );
    void itemHeadersReceived( const Akonadi::Item::List &items );
    void itemsReceived( const Akonadi::Item::List &items );
    void itemFetchResult( KJob* job );

  private:
    QStringList mSupportedMimeTypes;
    Akonadi::MimeTypeChecker mMimeTypeChecker;
    Akonadi::Collection::List mCollectionQueue;
    Akonadi::Collection mCurrentCollection;
    int mTotalAmount, mProcessedAmount, mPendingJobs;
};

#endif
