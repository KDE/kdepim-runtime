/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsattendee.h"

#include <QSharedData>
#include <QXmlStreamReader>

#include "ewsclient_debug.h"
#include "ewsmailbox.h"

using namespace Qt::StringLiterals;

class EwsAttendeePrivate : public QSharedData
{
public:
    EwsAttendeePrivate();
    virtual ~EwsAttendeePrivate();

    bool mValid;

    EwsMailbox mMailbox;
    EwsEventResponseType mResponse;
    QDateTime mResponseTime;
};

static constexpr auto responseTypeNames = std::to_array({
    "Unknown"_L1,
    "Organizer"_L1,
    "Tentative"_L1,
    "Accept"_L1,
    "Decline"_L1,
    "NoResponseReceived"_L1,
});

EwsAttendeePrivate::EwsAttendeePrivate()
    : mValid(false)
    , mResponse(EwsEventResponseNotReceived)
{
}

EwsAttendee::EwsAttendee()
    : d(new EwsAttendeePrivate())
{
}

EwsAttendee::EwsAttendee(QXmlStreamReader &reader)
    : d(new EwsAttendeePrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << u"Unexpected namespace in mailbox element:"_s << reader.namespaceUri();
            return;
        }
        const QStringView readerName = reader.name();
        if (readerName == "Mailbox"_L1) {
            d->mMailbox = EwsMailbox(reader);
            if (!d->mMailbox.isValid()) {
                qCWarning(EWSCLI_LOG) << u"Failed to read EWS request - invalid attendee %1 element."_s.arg(readerName.toString());
                return;
            }
        } else if (readerName == "ResponseType"_L1) {
            bool ok;
            d->mResponse = decodeEnumString<EwsEventResponseType>(reader.readElementText(), responseTypeNames, &ok);
            if (reader.error() != QXmlStreamReader::NoError || !ok) {
                qCWarning(EWSCLI_LOG) << u"Failed to read EWS request - invalid attendee %1 element."_s.arg(readerName.toString());
                return;
            }
        } else if (readerName == "LastResponseTime"_L1) {
            // Unsupported - ignore
            // qCWarningNC(EWSCLIENT_LOG) << u"Unsupported mailbox element %1"_s.arg(reader.name().toString());
            reader.skipCurrentElement();
        }
    }

    d->mValid = true;
}

EwsAttendeePrivate::~EwsAttendeePrivate() = default;

EwsAttendee::EwsAttendee(const EwsAttendee &other)
    : d(other.d)
{
}

EwsAttendee::EwsAttendee(EwsAttendee &&other)
    : d(std::move(other.d))
{
}

EwsAttendee::~EwsAttendee() = default;

EwsAttendee &EwsAttendee::operator=(const EwsAttendee &other)
{
    d = other.d;
    return *this;
}

EwsAttendee &EwsAttendee::operator=(EwsAttendee &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsAttendee::isValid() const
{
    return d->mValid;
}

const EwsMailbox &EwsAttendee::mailbox() const
{
    return d->mMailbox;
}

EwsEventResponseType EwsAttendee::response() const
{
    return d->mResponse;
}

QDateTime EwsAttendee::responseTime() const
{
    return d->mResponseTime;
}
