/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "resourcestateinterface.h"
#include "imapresource_debug.h"

ResourceStateInterface::~ResourceStateInterface()
{
}

QString ResourceStateInterface::mailBoxForCollection(const Akonadi::Collection &collection, bool showWarnings)
{
    if (collection.remoteId().isEmpty()) {   //This should never happen, investigate why a collection without remoteId made it this far
        if (showWarnings) {
            qCWarning(IMAPRESOURCE_LOG) << "Got incomplete ancestor chain due to empty remoteId:" << collection;
        }
        return QString();
    }

    if (collection.parentCollection() == Akonadi::Collection::root()) {
        /*if ( showWarnings  && collection.remoteId() != rootRemoteId())
          qCWarning(IMAPRESOURCE_LOG) << "RID mismatch, is " << collection.remoteId() << " expected " << rootRemoteId();
        */
        return QLatin1String("");   // see below, this intentionally not just QString()!
    }
    const QString parentMailbox = mailBoxForCollection(collection.parentCollection());
    if (parentMailbox.isNull()) { // invalid, != isEmpty() here!
        return QString();
    }

    const QString mailbox = parentMailbox + collection.remoteId();
    if (parentMailbox.isEmpty()) {
        return mailbox.mid(1);    // strip of the separator on top-level mailboxes
    }
    return mailbox;
}
