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

#include "kolabmessagehelper.h"

#include <collectionannotationsattribute.h>
#include "pimkolab/kolabformat/kolabdefinitions.h"

#include "kolabhelpers.h"
#include "kolabresource_debug.h"

KolabMessageHelper::KolabMessageHelper(const Akonadi::Collection &col)
    : mCollection(col)
{
}

KolabMessageHelper::~KolabMessageHelper()
{
}

Akonadi::Item KolabMessageHelper::createItemFromMessage(const KMime::Message::Ptr &message, const qint64 uid, const qint64 size, const QMap<QByteArray, QVariant> &attrs, const QList<QByteArray> &flags, const KIMAP::FetchJob::FetchScope &scope, bool &ok) const
{
    const Akonadi::Item item = MessageHelper::createItemFromMessage(message, uid, size, attrs, flags, scope, ok);
    if (!ok) {
        qCWarning(KOLABRESOURCE_LOG) << "Failed to read imap message";
        return item;
    }
    Kolab::FolderType folderType = Kolab::MailType;
    if (mCollection.hasAttribute<Akonadi::CollectionAnnotationsAttribute>()) {
        const QByteArray folderTypeString = KolabHelpers::getFolderTypeAnnotation(mCollection.attribute<Akonadi::CollectionAnnotationsAttribute>()->annotations());
        folderType = KolabHelpers::folderTypeFromString(folderTypeString);
    }
    return KolabHelpers::translateFromImap(folderType, item, ok);
}
