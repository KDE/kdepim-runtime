/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILMESSAGEHELPER_H
#define GMAILMESSAGEHELPER_H

#include <imap/messagehelper.h>

#include <AkonadiCore/Collection>

class ResourceTask;

class GmailMessageHelper : public MessageHelper
{
public:
    GmailMessageHelper(const Akonadi::Collection &collection, ResourceTask *currentTask);

    virtual Akonadi::Item createItemFromMessage(KMime::Message::Ptr message, const qint64 uid, const qint64 size, const QList<KIMAP::MessageAttribute> &attrs, const QList<QByteArray> &flags, const KIMAP::FetchJob::FetchScope &scope, bool &ok) const;

private:
    Akonadi::Collection mCollection;
    ResourceTask *mTask = nullptr;
};

#endif // GMAILMESSAGEHELPER_H
