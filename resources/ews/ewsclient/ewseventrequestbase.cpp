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

        if (reader.name() == QLatin1StringView("Notification")) {
            Notification nfy(reader);
            if (!nfy.isValid()) {
                setErrorMsg(u"Failed to process notification."_s);
                reader.skipCurrentElement();
                return;
            }
            mNotifications.append(nfy);
        } else if (reader.name() == QLatin1StringView("Notifications")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1StringView("Notification")) {
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
        } else if (reader.name() == QLatin1StringView("ConnectionStatus")) {
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
        {SubscriptionId, u"SubscriptionId"_s, &ewsXmlTextReader},
        {PreviousWatermark, u"PreviousWatermark"_s, &ewsXmlTextReader},
        {MoreEvents, u"MoreEvents"_s, &ewsXmlBoolReader},
        {Events, u"CopiedEvent"_s, &eventsReader},
        {Events, u"CreatedEvent"_s, &eventsReader},
        {Events, u"DeletedEvent"_s, &eventsReader},
        {Events, u"ModifiedEvent"_s, &eventsReader},
        {Events, u"MovedEvent"_s, &eventsReader},
        {Events, u"NewMailEvent"_s, &eventsReader},
        {Events, u"FreeBusyChangeEvent"_s, &eventsReader},
        {Events, u"StatusEvent"_s, &eventsReader},
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
        {Watermark, u"Watermark"_s, &ewsXmlTextReader},
        {Timestamp, u"TimeStamp"_s, &ewsXmlDateTimeReader},
        {FolderId, u"FolderId"_s, &ewsXmlIdReader},
        {ItemId, u"ItemId"_s, &ewsXmlIdReader},
        {ParentFolderId, u"ParentFolderId"_s, &ewsXmlIdReader},
        {OldFolderId, u"OldFolderId"_s, &ewsXmlIdReader},
        {OldItemId, u"OldItemId"_s, &ewsXmlIdReader},
        {OldParentFolderId, u"OldParentFolderId"_s, &ewsXmlIdReader},
        {UnreadCount, u"UnreadCount"_s, &ewsXmlUIntReader},
    };
    static const EventReader staticReader(items);

    EventReader ewsreader(staticReader);
    const QStringView elmName = reader.name();
    if (elmName == QLatin1StringView("CopiedEvent")) {
        mType = EwsCopiedEvent;
    } else if (elmName == QLatin1StringView("CreatedEvent")) {
        mType = EwsCreatedEvent;
    } else if (elmName == QLatin1StringView("DeletedEvent")) {
        mType = EwsDeletedEvent;
    } else if (elmName == QLatin1StringView("ModifiedEvent")) {
        mType = EwsModifiedEvent;
    } else if (elmName == QLatin1StringView("MovedEvent")) {
        mType = EwsMovedEvent;
    } else if (elmName == QLatin1StringView("NewMailEvent")) {
        mType = EwsNewMailEvent;
    } else if (elmName == QLatin1StringView("StatusEvent")) {
        mType = EwsStatusEvent;
    } else if (elmName == QLatin1StringView("FreeBusyChangedEvent")) {
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
