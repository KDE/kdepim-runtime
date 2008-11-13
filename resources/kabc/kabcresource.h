/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KABCRESOURCE
#define KABCRESOURCE

#include <akonadi/resourcebase.h>

namespace KABC {
  class ContactGroup;
  class DistributionList;
  class Resource;
  class ResourceABC;
}

class QTimer;

class KABCResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KABCResource( const QString &id );
    virtual ~KABCResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    virtual void doSetOnline( bool online );

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionChanged( const Akonadi::Collection &collection );

  private:
    class AddressBook;
    AddressBook *mAddressBook;
    KABC::Resource *mBaseResource;
    KABC::ResourceABC *mFolderResource;

    class ErrorHandler;
    ErrorHandler *mErrorHandler;

    bool mFullItemRetrieve;

    QTimer *mDelayedSaveTimer;

  private:
    void setResourcePointers( KABC::Resource *resource );

    bool openConfiguration();

    void closeConfiguration();

    bool saveAddressBook();

    bool scheduleSaveAddressBook();

    KABC::DistributionList *distListFromContactGroup( const KABC::ContactGroup& contactGroup );

    KABC::ContactGroup contactGroupFromDistList( const KABC::DistributionList* list ) const;

    typedef KABC::Resource Resource;

  private Q_SLOTS:
    void loadingError( Resource *resource, const QString &message );
    void initialLoadingFinished( Resource *resource );

    void addressBookChanged();

    void subResourceAdded( KABC::ResourceABC *resource,
                           const QString &type, const QString &subResource );
    void subResourceRemoved( KABC::ResourceABC *resource,
                             const QString &type, const QString &subResource );

    void subResourceChanged( KABC::ResourceABC *resource,
                             const QString &type, const QString &subResource );

    void reloadConfiguration();

    void delayedSaveAddressBook();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
