/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef ICALDIRRESOURCE_H
#define ICALDIRRESOURCE_H

#include <resourcebase.h>

#include <KCalCore/Incidence>

#include <QHash>

class ICalDirResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit ICalDirResource(const QString &id);
    ~ICalDirResource() override;

public Q_SLOTS:
    void configure(WId windowId) override;
    void aboutToQuit() override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &col) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionChanged(const Akonadi::Collection &collection) override;

private:
    bool loadIncidences();
    QString iCalDirectoryName() const;
    QString iCalDirectoryFileName(const QString &file) const;
    void initializeICalDirectory() const;

private:
    QHash<QString, KCalCore::Incidence::Ptr> mIncidences;
};

#endif
