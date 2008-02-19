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

#include <resourcebase.h>

namespace KABC {
  class Resource;
}

class QTimer;

class KABCResource : public Akonadi::ResourceBase
{
  Q_OBJECT

  public:
    KABCResource( const QString &id );
    virtual ~KABCResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col, const QStringList &parts );
    bool retrieveItem( const Akonadi::Item &item, const QStringList &parts );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::DataReference &ref );

  private:
    class AddressBook;
    AddressBook *mAddressBook;
    KABC::Resource *mResource;
    bool mLoaded;
    bool mExplicitLoading;

    QTimer *mDelayedUpdateTimer;

    class ErrorHandler;
    ErrorHandler *mErrorHandler;

    typedef KABC::Resource Resource;
  private Q_SLOTS:
    void loadingFinished( Resource *resource );
    void loadingError( Resource *resource, const QString &message );
    void addressBookChanged();
    void delayedUpdate();
};

#endif
