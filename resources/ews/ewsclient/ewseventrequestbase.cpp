/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewseventrequestbase.h"
#include "ewsclient_debug.h"
#include "ewsxml.h"

using namespace Qt::StringLiterals;

enum NotificationElementType {
    InvalidNotificationElement = -1,
    SubscriptionId,
    PreviousWatermark,
    MoreEvents,
    Events,
};
using NotificationReader = EwsXml<NotificationElementType>;

enum EventElementType {
    InvalidEventElement = -1,
    Watermark,
    Timestamp,
    ItemId,
    FolderId,
    ParentFolderId,
    OldItemId,
    OldFolderId,
    OldParentFolderId,
    UnreadCount,
};
using EventReader = EwsXml<EventElementType>;

EwsEventRequestBase::EwsEventRequestBase(EwsClient &client, const QString &reqName, QObject *parent)
    : EwsRequest(client, parent)
    , mReqName(reqName)
{
    qRegisterMetaType<EwsEventRequestBase::Event::List>();
}

EwsEventRequestBase::~EwsEventRequestBase() = default;

bool EwsEventRequestBase::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, mReqName, [this](QXmlStreamReader &reader) {
        return parseNotificationsResponse(reader);
    });
}

bool EwsEventRequestBase::parseNotificationsResponse(QXmlStreamReader &reader)
{
    Response resp(reader);
    if (resp.responseClass() == EwsResponseUnknown) {
        return false;
    }

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp.isSuccess()) {
            int numEv = 0;
            const auto notifications = resp.notifications();
            for (const Notification &nfy : notifications) {
                numEv += nfy.events().size();
            }
            qCDebugNC(EWSCLI_REQUEST_LOG) << "Got" << mReqName << "response (" << resp.notifications().size() << "notifications," << numEv << "events)";
        } else {
            qCDebug(EWSCLI_REQUEST_LOG) << "Got" << mReqName << "response -" << resp.responseMessage();
        }
    }

    mResponses.append(resp);
    return true;
}

EwsEventRequestBase::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsMsgNsUri && reader.namespaceUri() != ewsTypeNsUri) {
            setErrorMsg(u"Unexpected namespace in %1 element: %2"_s.arg(u"ResponseMessage"_s, reader.namespaceUri().toString()));
            return;
        }

        if (reader.name() == "Notification"_L1) {
            Notification nfy(reader);
            if (!nfy.isValid()) {
                setErrorMsg(u"Failed to process notification."_s);
                reader.skipCurrentElement();
                return;
            }
            mNotifications.append(nfy);
        } else if (reader.name() == "Notifications"_L1) {
            while (reader.readNextStartElement()) {
                if (reader.name() == "Notification"_L1) {
                    Notification nfy(reader);
                    if (!nfy.isValid()) {
                        setErrorMsg(u"Failed to process notification."_s);
                        reader.skipCurrentElement();
                        return;
                    }
                    mNotifications.append(nfy);
                } else {
                    setErrorMsg(u"Failed to read EWS request - expected Notification inside Notifications"_s);
                }
            }
        } else if (reader.name() == "ConnectionStatus"_L1) {
            reader.skipCurrentElement();
        } else if (!readResponseElement(reader)) {
            setErrorMsg(u"Failed to read EWS request - invalid response element '%1'"_s.arg(reader.name().toString()));
            return;
        }
    }
}

EwsEventRequestBase::Notification::Notification(QXmlStreamReader &reader)
{
    static const QList<NotificationReader::Item> items = {
        {SubscriptionId, "SubscriptionId"_L1, &ewsXmlTextReader},
        {PreviousWatermark, "PreviousWatermark"_L1, &ewsXmlTextReader},
        {MoreEvents, "MoreEvents"_L1, &ewsXmlBoolReader},
        {Events, "CopiedEvent"_L1, &eventsReader},
        {Events, "CreatedEvent"_L1, &eventsReader},
        {Events, "DeletedEvent"_L1, &eventsReader},
        {Events, "ModifiedEvent"_L1, &eventsReader},
        {Events, "MovedEvent"_L1, &eventsReader},
        {Events, "NewMailEvent"_L1, &eventsReader},
        {Events, "FreeBusyChangeEvent"_L1, &eventsReader},
        {Events, "StatusEvent"_L1, &eventsReader},
    };
    static const NotificationReader staticReader(items);

    NotificationReader ewsreader(staticReader);

    if (!ewsreader.readItems(reader, ewsTypeNsUri)) {
        return;
    }

    QHash<NotificationElementType, QVariant> values = ewsreader.values();

    mSubscriptionId = values[SubscriptionId].toString();
    mWatermark = values[PreviousWatermark].toString();
    mMoreEvents = values[MoreEvents].toBool();
    mEvents = values[Events].value<Event::List>();
}

bool EwsEventRequestBase::Notification::eventsReader(QXmlStreamReader &reader, QVariant &val)
{
    Event::List events = val.value<Event::List>();
    const QString elmName(reader.name().toString());

    Event event(reader);
    if (!event.isValid()) {
        qCWarningNC(EWSCLI_LOG) << u"Failed to read %1 element"_s.arg(elmName);
        return false;
    }

    events.append(event);

    val = QVariant::fromValue<Event::List>(events);
    return true;
}

EwsEventRequestBase::Event::Event(QXmlStreamReader &reader)
    : mType(EwsUnknownEvent)
{
    static const QList<EventReader::Item> items = {
        {Watermark, "Watermark"_L1, &ewsXmlTextReader},
        {Timestamp, "TimeStamp"_L1, &ewsXmlDateTimeReader},
        {FolderId, "FolderId"_L1, &ewsXmlIdReader},
        {ItemId, "ItemId"_L1, &ewsXmlIdReader},
        {ParentFolderId, "ParentFolderId"_L1, &ewsXmlIdReader},
        {OldFolderId, "OldFolderId"_L1, &ewsXmlIdReader},
        {OldItemId, "OldItemId"_L1, &ewsXmlIdReader},
        {OldParentFolderId, "OldParentFolderId"_L1, &ewsXmlIdReader},
        {UnreadCount, "UnreadCount"_L1, &ewsXmlUIntReader},
    };
    static const EventReader staticReader(items);

    EventReader ewsreader(staticReader);
    const QStringView elmName = reader.name();
    if (elmName == "CopiedEvent"_L1) {
        mType = EwsCopiedEvent;
    } else if (elmName == "CreatedEvent"_L1) {
        mType = EwsCreatedEvent;
    } else if (elmName == "DeletedEvent"_L1) {
        mType = EwsDeletedEvent;
    } else if (elmName == "ModifiedEvent"_L1) {
        mType = EwsModifiedEvent;
    } else if (elmName == "MovedEvent"_L1) {
        mType = EwsMovedEvent;
    } else if (elmName == "NewMailEvent"_L1) {
        mType = EwsNewMailEvent;
    } else if (elmName == "StatusEvent"_L1) {
        mType = EwsStatusEvent;
    } else if (elmName == "FreeBusyChangedEvent"_L1) {
        mType = EwsFreeBusyChangedEvent;
    } else {
        qCWarning(EWSCLI_LOG) << u"Unknown notification event type: %1"_s.arg(elmName.toString());
        return;
    }

    if (!ewsreader.readItems(reader, ewsTypeNsUri)) {
        mType = EwsUnknownEvent;
        return;
    }

    QHash<EventElementType, QVariant> values = ewsreader.values();

    mWatermark = values[Watermark].toString();
    mTimestamp = values[Timestamp].toDateTime();
    if (values.contains(ItemId)) {
        mId = values[ItemId].value<EwsId>();
        mOldId = values[OldItemId].value<EwsId>();
        mIsFolder = false;
    } else {
        mId = values[FolderId].value<EwsId>();
        mOldId = values[OldFolderId].value<EwsId>();
        mIsFolder = true;
    }
    mParentFolderId = values[ParentFolderId].value<EwsId>();
    mOldParentFolderId = values[OldParentFolderId].value<EwsId>();
    mUnreadCount = values[UnreadCount].toUInt();

    if (mType == EwsStatusEvent) {
        qCDebugNCS(EWSCLI_LOG) << elmName.toString();
    } else {
        qCDebugNCS(EWSCLI_LOG).nospace() << elmName.toString() << ", " << (mIsFolder ? 'F' : 'I') << ", parent: " << mParentFolderId << u", id: "_s << mId;
    }
}

bool EwsEventRequestBase::Response::operator==(const Response &other) const
{
    return mNotifications == other.mNotifications;
}

bool EwsEventRequestBase::Notification::operator==(const Notification &other) const
{
    return (mSubscriptionId == other.mSubscriptionId) && (mWatermark == other.mWatermark) && (mMoreEvents == other.mMoreEvents) && (mEvents == other.mEvents);
}

bool EwsEventRequestBase::Event::operator==(const Event &other) const
{
    return (mType == other.mType) && (mWatermark == other.mWatermark) && (mTimestamp == other.mTimestamp) && (mId == other.mId)
        && (mParentFolderId == other.mParentFolderId) && (mUnreadCount == other.mUnreadCount) && (mOldId == other.mOldId)
        && (mOldParentFolderId == other.mOldParentFolderId) && (mIsFolder == other.mIsFolder);
}

#include "moc_ewseventrequestbase.cpp"
