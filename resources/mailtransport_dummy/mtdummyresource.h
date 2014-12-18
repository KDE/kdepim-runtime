/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as published
    by the Free Software Foundation; either version 2 of the License or
    ( at your option ) version 3 or, at the discretion of KDE e.V.
    ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MTDUMMYRESOURCE_H
#define MTDUMMYRESOURCE_H

#include <Item>
#include <ResourceBase>
#include <TransportResourceBase>

class MTDummyResource : public Akonadi::ResourceBase, public Akonadi::TransportResourceBase
{
    Q_OBJECT

public:
    explicit MTDummyResource(const QString &id);
    ~MTDummyResource();

public:
    void configure(WId windowId) Q_DECL_OVERRIDE;

    /* reimpl from ResourceBase::Transport */
    void sendItem(const Akonadi::Item &message) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &col) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

protected:
    void aboutToQuit() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void jobResult(KJob *job);

private:
    Akonadi::Item::Id currentlySending;

};

#endif
