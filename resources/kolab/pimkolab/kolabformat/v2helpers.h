/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "kolabdefinitions.h"

#include "kolabformat/errorhandler.h"
#include "kolabformatV2/contact.h"
#include "kolabformatV2/distributionlist.h"
#include "kolabformatV2/event.h"
#include "kolabformatV2/journal.h"
#include "kolabformatV2/kolabbase.h"
#include "kolabformatV2/task.h"
#include "mime/mimeutils.h"

#include <KCalendarCore/Incidence>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>
#include <KMime/Message>

#include <qdom.h>

namespace Kolab
{
/*
 * Parse XML, create KCalendarCore container and extract attachments
 */
template<typename KCalPtr, typename Container>
static KCalPtr fromXML(const QByteArray &xmlData, QStringList &attachments)
{
    const QDomDocument xmlDoc = KolabV2::KolabBase::loadDocument(QString::fromUtf8(xmlData)); // TODO extract function from V2 format
    if (xmlDoc.isNull()) {
        Critical() << "Failed to read the xml document";
        return KCalPtr();
    }
    const KCalPtr i = Container::fromXml(xmlDoc, QString()); // For parsing we don't need the timezone, so we don't set one
    Q_ASSERT(i);
    const QDomNodeList nodes = xmlDoc.elementsByTagName(QStringLiteral("inline-attachment"));
    for (int i = 0; i < nodes.size(); i++) {
        attachments.append(nodes.at(i).toElement().text());
    }
    return i;
}

void getAttachments(KCalendarCore::Incidence::Ptr incidence, const QStringList &attachments, const std::shared_ptr<KMime::Message> &mimeData);

template<typename IncidencePtr, typename Converter>
static inline IncidencePtr incidenceFromKolabImpl(const std::shared_ptr<KMime::Message> &data, const QByteArray &mimetype, const QString &timezoneId)
{
    Q_UNUSED(timezoneId)
    KMime::Content *xmlContent = Mime::findContentByType(data, mimetype);
    if (!xmlContent) {
        Critical() << "couldn't find part";
        return IncidencePtr();
    }
    const QByteArray &xmlData = xmlContent->decodedBody();

    QStringList attachments;
    IncidencePtr ptr = fromXML<IncidencePtr, Converter>(xmlData, attachments); // TODO do we care about timezone?
    getAttachments(ptr, attachments, data);

    return ptr;
}

KContacts::Addressee addresseeFromKolab(const QByteArray &xmlData, const std::shared_ptr<KMime::Message> &data);
KContacts::Addressee addresseeFromKolab(const QByteArray &xmlData, QString &pictureAttachmentName, QString &logoAttachmentName, QString &soundAttachmentName);

std::shared_ptr<KMime::Message> contactToKolabFormat(const KolabV2::Contact &contact, const QString &productId);

KContacts::ContactGroup contactGroupFromKolab(const QByteArray &xmlData);

std::shared_ptr<KMime::Message> distListToKolabFormat(const KolabV2::DistributionList &distList, const QString &productId);

QStringList readLegacyDictionaryConfiguration(const QByteArray &xmlData, QString &language);
}
