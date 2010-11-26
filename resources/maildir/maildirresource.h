/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef __MAILDIR_RESOURCE_H__
#define __MAILDIR_RESOURCE_H__

#include <akonadi/collection.h>
#include <akonadi/resourcebase.h>

class MaildirSettings;
namespace KPIM
{
class Maildir;
}

class MaildirResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    MaildirResource( const QString &id );
    ~MaildirResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual QString itemMimeType();

    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest );
    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

  private slots:
    void configurationChanged();

  private:
    void ensureDirExists();
    bool ensureSaneConfiguration();
    Akonadi::Collection::List listRecursive( const Akonadi::Collection &root, const KPIM::Maildir &dir );
    /** Creates a maildir object for the collection @p col, given it has the full ancestor chain set. */
    KPIM::Maildir maildirForCollection( const Akonadi::Collection &col ) const;

  private:
    MaildirSettings *mSettings;

};

#endif
