/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "messagehelper.h"

#include <Akonadi/KMime/MessageFlags>

#include "resourcetask.h"

MessageHelper::~MessageHelper()
{

}

Akonadi::Item MessageHelper::createItemFromMessage(KMime::Message::Ptr message,
                                                   const qint64 uid,
                                                   const qint64 size,
                                                   const QList<KIMAP::MessageAttribute> &attrs,
                                                   const QList<QByteArray> &flags,
                                                   const KIMAP::FetchJob::FetchScope &scope,
                                                   bool &ok) const
{
    Q_UNUSED(attrs);

    Akonadi::Item i;
    if (scope.mode == KIMAP::FetchJob::FetchScope::Flags) {
        i.setRemoteId(QString::number(uid));
        i.setMimeType(KMime::Message::mimeType());
        i.setFlags(Akonadi::Item::Flags::fromList(ResourceTask::toAkonadiFlags(flags)));
    } else {
        if (!message) {
            kWarning() << "Got empty message: " << uid;
            ok = false;
            return Akonadi::Item();
        }
        // Sometimes messages might not have a body at all
        if (message->body().isEmpty() && (scope.mode == KIMAP::FetchJob::FetchScope::Full || scope.mode == KIMAP::FetchJob::FetchScope::Content)) {
            // In that case put a space in as body so that it gets cached
            // otherwise we'll wrongly believe the body part is missing from the cache
            message->setBody( " " );
        }
        i.setRemoteId(QString::number(uid));
        i.setMimeType(KMime::Message::mimeType());
        i.setPayload(KMime::Message::Ptr(message));
        i.setSize(size);

        // update status flags
        if (KMime::isSigned(message.get())) {
            i.setFlag(Akonadi::MessageFlags::Signed);
        }
        if (KMime::isEncrypted(message.get())) {
            i.setFlag(Akonadi::MessageFlags::Encrypted);
        }
        if (KMime::isInvitation(message.get())) {
            i.setFlag(Akonadi::MessageFlags::HasInvitation);
        }
        if (KMime::hasAttachment(message.get())) {
            i.setFlag(Akonadi::MessageFlags::HasAttachment);
        }

        foreach (const QByteArray &flag, ResourceTask::toAkonadiFlags(flags)) {
            i.setFlag(flag);
        }
    }
    ok = true;
    return i;
}
