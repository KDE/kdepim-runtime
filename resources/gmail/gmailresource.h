/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
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

#ifndef GMAILRESOURCE_H
#define GMAILRESOURCE_H

#include <imap/imapresourcebase.h>

class GmailResource : public ImapResourceBase
{
    Q_OBJECT

public:
    explicit GmailResource(const QString &id);
    ~GmailResource();

    KDialog *createConfigureDialog (WId windowId);
    QString defaultName() const;

    ResourceStateInterface::Ptr createResourceState (const TaskArguments &args);

    void retrieveCollections();
    void retrieveItems(const Akonadi::Collection &col);

    void linkItems(const QByteArray &collectionName, const QStringList &remoteIds);

private Q_SLOTS:
    void onConfigurationDone(int result);
    void onRetrieveItemsCollectionRetrieved(KJob *job);

private:
};



#endif // GMAILRESOURCE_H
