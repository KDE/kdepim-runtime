/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewscalendarhandler.h"

#include <KCalendarCore/Event>

#include "ewscreatecalendarjob.h"
#include "ewsfetchcalendardetailjob.h"
#include "ewsmodifycalendarjob.h"
#include "ewsresource_debug.h"

using namespace Akonadi;

EwsCalendarHandler::EwsCalendarHandler() = default;

EwsCalendarHandler::~EwsCalendarHandler() = default;

EwsItemHandler *EwsCalendarHandler::factory()
{
    return new EwsCalendarHandler();
}

EwsFetchItemDetailJob *EwsCalendarHandler::fetchItemDetailJob(EwsClient &client, QObject *parent, const Akonadi::Collection &collection)
{
    return new EwsFetchCalendarDetailJob(client, parent, collection);
}

void EwsCalendarHandler::setSeenFlag(Item &item, bool value)
{
    Q_UNUSED(item)
    Q_UNUSED(value)
}

QString EwsCalendarHandler::mimeType()
{
    return KCalendarCore::Event::eventMimeType();
}

bool EwsCalendarHandler::setItemPayload(Akonadi::Item &item, const EwsItem &ewsItem)
{
    /* Akonadi requests the payload of an item that is not cached. Returning without one
     * made it store the item empty, so that calendar applications did not show it. */
    const EwsCalendarIncidence picked = EwsFetchCalendarDetailJob::incidenceFromItem(ewsItem);
    if (!picked.incidence) {
        qCWarningNC(EWSRES_LOG) << QStringLiteral("Empty calendar or failed to parse it.");
        return false;
    }
    item.setPayload<KCalendarCore::Incidence::Ptr>(picked.incidence);
    return true;
}

QList<EwsPropertyField> EwsCalendarHandler::payloadProperties()
{
    // Needed for the exclusions of a series, see ewsPrepareForStorage().
    return {EwsPropertyField(QStringLiteral("calendar:ModifiedOccurrences")), EwsPropertyField(QStringLiteral("calendar:DeletedOccurrences"))};
}

EwsModifyItemJob *EwsCalendarHandler::modifyItemJob(EwsClient &client, const QList<Akonadi::Item> &items, const QSet<QByteArray> &parts, QObject *parent)
{
    return new EwsModifyCalendarJob(client, items, parts, parent);
}

EwsCreateItemJob *EwsCalendarHandler::createItemJob(EwsClient &client,
                                                    const Akonadi::Item &item,
                                                    const Akonadi::Collection &collection,
                                                    EwsTagStore *tagStore,
                                                    EwsResource *parent)
{
    return new EwsCreateCalendarJob(client, item, collection, tagStore, parent);
}

EWS_DECLARE_ITEM_HANDLER(EwsCalendarHandler, EwsItemTypeCalendarItem)
