/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfetchcalendardetailjob.h"

#include <algorithm>

#include <QTimeZone>

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include "ewsgetitemrequest.h"
#include "ewsitemshape.h"
#include "ewsoccurrenceexclusions.h"
#include "ewsresource_debug.h"

using namespace Akonadi;

namespace
{

QString sanitizeEwsIcal(QString content)
{
    // It is a known bug with some versions of EWS that in some cases, the iCal data are missing the trailing
    // END:VCALENDAR, which causes ical to fail  to parse the data. This is a hacky workaround to be able to
    // deal with this situation.
    // https://answers.microsoft.com/en-us/outlook_com/forum/all/ics-files-are-missing-end-nodes/fb2e1cc7-eed5-43e2-a62f-bebc653e119f?page=2
    // https://github.com/asciipip/mutt-ical/issues/4
    if (!content.contains(u"END:VCALENDAR")) {
        content += QStringLiteral("\nEND:VCALENDAR");
    }

    return content;
}

} // namespace

EwsFetchCalendarDetailJob::EwsFetchCalendarDetailJob(EwsClient &client, QObject *parent, const Collection &collection)
    : EwsFetchItemDetailJob(client, parent, collection)
{
    EwsItemShape shape(EwsShapeIdOnly);
    /*    shape << EwsPropertyField(QStringLiteral("calendar:UID"));
        shape << EwsPropertyField(QStringLiteral("item:Subject"));
        shape << EwsPropertyField(QStringLiteral("item:Body"));
        shape << EwsPropertyField(QStringLiteral("calendar:Organizer"));
        shape << EwsPropertyField(QStringLiteral("calendar:RequiredAttendees"));
        shape << EwsPropertyField(QStringLiteral("calendar:OptionalAttendees"));
        shape << EwsPropertyField(QStringLiteral("calendar:Resources"));
        shape << EwsPropertyField(QStringLiteral("calendar:Start"));
        shape << EwsPropertyField(QStringLiteral("calendar:End"));
        shape << EwsPropertyField(QStringLiteral("calendar:IsAllDayEvent"));
        shape << EwsPropertyField(QStringLiteral("calendar:LegacyFreeBusyStatus"));
        shape << EwsPropertyField(QStringLiteral("calendar:AppointmentSequenceNumber"));
        shape << EwsPropertyField(QStringLiteral("calendar:IsRecurring"));
        shape << EwsPropertyField(QStringLiteral("calendar:Recurrence"));
        shape << EwsPropertyField(QStringLiteral("calendar:FirstOccurrence"));
        shape << EwsPropertyField(QStringLiteral("calendar:LastOccurrence"));
        shape << EwsPropertyField(QStringLiteral("calendar:ModifiedOccurrences"));
        shape << EwsPropertyField(QStringLiteral("calendar:DeletedOccurrences"));
        shape << EwsPropertyField(QStringLiteral("calendar:StartTimeZone"));
        shape << EwsPropertyField(QStringLiteral("calendar:EndTimeZone"));
        shape << EwsPropertyField(QStringLiteral("calendar:MyResponseType"));
        shape << EwsPropertyField(QStringLiteral("item:HasAttachments"));
        shape << EwsPropertyField(QStringLiteral("item:Attachments"));*/

    //    shape << EwsPropertyField(QStringLiteral("item:Attachments"));
    shape << EwsPropertyField(QStringLiteral("calendar:ModifiedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("calendar:DeletedOccurrences"));
    shape << EwsPropertyField(QStringLiteral("item:Body"));
    //    shape << EwsPropertyField(QStringLiteral("item:Culture"));
    shape << EwsPropertyField(QStringLiteral("item:MimeContent"));
    shape << EwsPropertyField(QStringLiteral("item:Subject"));
    //    shape << EwsPropertyField(QStringLiteral("calendar:TimeZone"));
    mRequest->setItemShape(shape);
}

EwsFetchCalendarDetailJob::~EwsFetchCalendarDetailJob() = default;

void EwsFetchCalendarDetailJob::processItems(const EwsGetItemRequest::Response::List &responses)
{
    KCalendarCore::ICalFormat format;

    EwsId::List addItems;

    for (const EwsGetItemRequest::Response &resp : responses) {
        if (!resp.isSuccess()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch item: %1 (%2)").arg(resp.responseMessage(), resp.responseCode());
            continue;
        }

        const EwsItem &ewsItem = resp.item();
        const EwsId remoteId = ewsItem[EwsItemFieldItemId].value<EwsId>();

        // The order of the responses is not the order of the request, so the item
        // a response belongs to is looked up by its id.
        auto itemIt = std::find_if(mChangedItems.begin(), mChangedItems.end(), [&remoteId](const Item &item) {
            return item.remoteId() == remoteId.id();
        });
        if (itemIt == mChangedItems.end()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Got a response for an item that was not requested: %1").arg(remoteId.id());
            continue;
        }

        const QString mimeContent = sanitizeEwsIcal(ewsItem[EwsItemFieldMimeContent].toString());

        KCalendarCore::Calendar::Ptr memcal(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
        format.fromString(memcal, mimeContent);
        const auto events{memcal->events()};
        qCDebugNC(EWSRES_LOG) << QStringLiteral("Found %1 events").arg(events.count());

        EwsCalendarIncidence picked = ewsPickIncidence(memcal);
        if (!picked.incidence) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Empty calendar or failed to parse it.");
            continue;
        }
        if (!picked.isMaster) {
            qCDebugNC(EWSRES_LOG)
                << QStringLiteral("No master event in the calendar data of item %1 - storing the occurrence as it is").arg(itemIt->remoteId());
        }
        KCalendarCore::Incidence::Ptr incidence = picked.incidence;

        QDateTime dt(incidence->dtStart());
        if (dt.isValid()) {
            incidence->setDtStart(dt);
        }
        if (incidence->type() == KCalendarCore::Incidence::TypeEvent) {
            auto event = reinterpret_cast<KCalendarCore::Event *>(incidence.data());
            dt = event->dtEnd();
            if (dt.isValid()) {
                event->setDtEnd(dt);
            }
        }
        dt = incidence->recurrenceId();
        if (dt.isValid()) {
            incidence->setRecurrenceId(dt);
        }

        // A cancelled occurrence leaves an empty slot that only the master can be
        // stopped from generating; a moved one is delivered as an item of its own.
        ewsPrepareForStorage(ewsItem, picked);

        itemIt->setPayload<KCalendarCore::Incidence::Ptr>(incidence);

        if (picked.isMaster) {
            for (const EwsMovedOccurrence &moved : ewsMovedOccurrences(ewsItem)) {
                mExpectedExceptions.insert(moved.itemId.id(), ExpectedException{moved.originalStart, incidence->uid()});
                addItems.append(moved.itemId);
            }
        }
    }

    if (addItems.isEmpty()) {
        emitResult();
        return;
    }

    auto req = new EwsGetItemRequest(mClient, this);
    EwsItemShape shape(EwsShapeIdOnly);
    //        shape << EwsPropertyField(QStringLiteral("item:Attachments"));
    shape << EwsPropertyField(QStringLiteral("item:Body"));
    shape << EwsPropertyField(QStringLiteral("item:MimeContent"));
    //        shape << EwsPropertyField(QStringLiteral("calendar:TimeZone"));
    //        shape << EwsPropertyField(QStringLiteral("item:Culture"));
    req->setItemShape(shape);

    req->setItemIds(addItems);
    connect(req, &KJob::result, this, &EwsFetchCalendarDetailJob::exceptionItemsFetched);
    req->start();
}

void EwsFetchCalendarDetailJob::exceptionItemsFetched(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    auto req = qobject_cast<EwsGetItemRequest *>(job);

    if (!req) {
        setError(1);
        setErrorText(QStringLiteral("Job is not an instance of EwsGetItemRequest"));
        emitResult();
        return;
    }

    KCalendarCore::ICalFormat format;
    const auto responses{req->responses()};
    for (const EwsGetItemRequest::Response &resp : responses) {
        if (!resp.isSuccess()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Failed to fetch item.");
            continue;
        }
        const EwsItem &ewsItem = resp.item();

        Item item(KCalendarCore::Event::eventMimeType());
        item.setParentCollection(mCollection);
        auto id = ewsItem[EwsItemFieldItemId].value<EwsId>();
        item.setRemoteId(id.id());
        item.setRemoteRevision(id.changeKey());

        const QString mimeContent = sanitizeEwsIcal(ewsItem[EwsItemFieldMimeContent].toString());
        KCalendarCore::Calendar::Ptr memcal(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
        format.fromString(memcal, mimeContent);
        const auto events{memcal->events()};
        if (events.isEmpty()) {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("Empty calendar or failed to parse it.");
            continue;
        }

        // The occurrence asked for is the event whose RECURRENCE-ID is the original
        // start of the slot it was moved away from.
        const ExpectedException expected = mExpectedExceptions.value(id.id());
        KCalendarCore::Event::Ptr occurrenceEvent;
        if (expected.originalStart.isValid()) {
            for (const KCalendarCore::Event::Ptr &candidate : events) {
                if (candidate->recurrenceId() == expected.originalStart) {
                    occurrenceEvent = candidate;
                    break;
                }
            }
            if (!occurrenceEvent) {
                qCWarningNC(EWSRES_LOG) << QStringLiteral("No occurrence of item %1 starts at %2").arg(id.id(), expected.originalStart.toString(Qt::ISODate));
                continue;
            }
        } else {
            qCWarningNC(EWSRES_LOG) << QStringLiteral("No original start known for item %1 - taking the last event of its calendar data").arg(id.id());
            occurrenceEvent = events.last();
        }

        if (!expected.masterUid.isEmpty() && occurrenceEvent->uid() != expected.masterUid) {
            qCWarningNC(EWSRES_LOG)
                << QStringLiteral("Occurrence of item %1 belongs to %2 and not to the master %3").arg(id.id(), occurrenceEvent->uid(), expected.masterUid);
        }

        KCalendarCore::Incidence::Ptr incidence(occurrenceEvent);
        incidence->clearRecurrence();

        QDateTime dt(incidence->dtStart());
        if (dt.isValid()) {
            incidence->setDtStart(dt);
        }
        if (incidence->type() == KCalendarCore::Incidence::TypeEvent) {
            auto event = reinterpret_cast<KCalendarCore::Event *>(incidence.data());
            dt = event->dtEnd();
            if (dt.isValid()) {
                event->setDtEnd(dt);
            }
        }
        dt = incidence->recurrenceId();
        if (dt.isValid()) {
            incidence->setRecurrenceId(dt);
        }

        item.setPayload<KCalendarCore::Incidence::Ptr>(incidence);

        mChangedItems.append(item);
    }

    emitResult();
}

#include "moc_ewsfetchcalendardetailjob.cpp"
