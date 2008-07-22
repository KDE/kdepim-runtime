/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007 <j.kerihuel@openchange.org>.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef OCRESOURCE_H
#define OCRESOURCE_H

#include <akonadi/resourcebase.h>

#include <libmapi++/libmapi++.h>

class OCResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    OCResource( const QString &id );
    ~OCResource();

public Q_SLOTS:
    virtual void configure( WId windowId );

protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

private:
    void login();
    void appendMessageToItem( libmapipp::message & mapi_message, Akonadi::Item & item );
    void appendContactToItem( libmapipp::message & mapi_message, Akonadi::Item & item );
    void appendJournalToItem( libmapipp::message & mapi_message, Akonadi::Item & item );
    void appendEventToItem( libmapipp::message & mapi_message, Akonadi::Item & item );
    void appendTodoToItem( libmapipp::message & mapi_message, Akonadi::Item & item );

    QString mimeTypeForFolderType( const char *folderTypeValue ) const;

    // this method may recurse into itself.
    void getChildFolders( libmapipp::folder& parentFolder,
                          const Akonadi::Collection &parentCollection,
                          Akonadi::Collection::List &collections);

    QString m_profileName;
    libmapipp::session *m_session;
    QString m_profileDatabase; // TODO: maybe this should be a constructor arg?

    friend class ProfileDialog;
};

#endif
