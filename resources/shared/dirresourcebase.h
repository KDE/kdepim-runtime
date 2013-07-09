/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef DIRRESOURCEBASE_H
#define DIRRESOURCEBASE_H

#include <Akonadi/ResourceBase>
#include <Akonadi/AgentBase>

#include <KDirWatch>
#include <QtCore/QStringList>

class DirResourceBase : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT

  public:
    enum FileEvent {
        FileCreated,
        FileModified,
        FileDeleted
    };

    explicit DirResourceBase( const QString &id );
    virtual ~DirResourceBase();

  public Q_SLOTS:
    void configure( WId windowId );
    void aboutToQuit();

  protected Q_SLOTS:
    virtual void retrieveCollections() = 0;
    virtual void retrieveItems( const Akonadi::Collection &col ) = 0;
    virtual bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts ) = 0;

    virtual void slotDirChanged( const QString &path ) = 0;
    virtual void slotFileCreated( const QString &path ) = 0;
    virtual void slotFileDeleted( const QString &path ) = 0;

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection ) = 0;
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts ) = 0;
    virtual void itemRemoved( const Akonadi::Item &item ) = 0;

    virtual QString mimeType() const = 0;
    virtual QString loadEntity( const QString &filePath ) = 0;
    virtual void clear();

    QString directoryName() const;
    QString directoryFileName( const QString &file ) const;
    Akonadi::Collection createCollection() const;

    bool ensureReady( const QString &path, DirResourceBase::FileEvent event );
    bool loadEntities();
    void initializeDirectory();

  protected:
    KDirWatch *mDirWatch;
    QStringList mIgnoredPaths;

    Akonadi::Collection mCollection;

  private Q_SLOTS:
    void slotCollectionRetrieved( KJob *job );

  private:
    void initializeDirWatch();
};

#endif // DIRRESOURCEBASE_H
