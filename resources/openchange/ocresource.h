/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007.

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

#include <resourcebase.h>
#include <QtDBus/QDBusMessage>

extern "C" {
#include <libmapi/libmapi.h>
#include <talloc.h>
}

#include <QDialog>
#include <QListWidget>
class OCResource;

class ProfileDialog : public QDialog
{
  Q_OBJECT

  public:
    ProfileDialog( OCResource *resource, QWidget *parent = 0 );

  private Q_SLOTS:
    void addNewProfile();
    void deleteSelectedProfile();
    void editExistingProfile();
    void setAsDefaultProfile();

  private:
    void fillProfileList();

    OCResource *m_resource;
    QListWidget *m_listOfProfiles;
};


class OCResource : public Akonadi::ResourceBase
{
    Q_OBJECT

public:
    OCResource( const QString &id );
    ~OCResource();

public Q_SLOTS:
    virtual bool requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg );
    virtual void configure();

protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::DataReference &ref );

    void retrieveCollections();
    void synchronizeCollection( const Akonadi::Collection &col );

private:
    enum MAPISTATUS fetchFolder( const Akonadi::Collection &collection );

    // this method may recurse into itself.
    void getChildFolders( mapi_object_t *parentFolder, mapi_id_t id,
                          const Akonadi::Collection &parentCollection,
                          Akonadi::Collection::List &collections);

    mapi_object_t m_mapiStore;
    struct mapi_session *m_session;
    TALLOC_CTX *m_mapiMemoryContext;
    QString m_profileDatabase; // TODO: maybe this should be a constructor arg?

    friend class ProfileDialog;
};

#endif
