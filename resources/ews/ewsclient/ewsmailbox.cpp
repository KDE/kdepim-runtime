/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsmailbox.h"

#include <QSharedData>

#include <KMime/Types>

#include "ewsclient_debug.h"

class EwsMailboxPrivate : public QSharedData
{
public:
    EwsMailboxPrivate();
    virtual ~EwsMailboxPrivate();

    bool mValid;

    QString mName;
    QString mEmail;
};

EwsMailboxPrivate::EwsMailboxPrivate()
    : mValid(false)
{
}

EwsMailbox::EwsMailbox()
    : d(new EwsMailboxPrivate())
{
}

EwsMailbox::EwsMailbox(QXmlStreamReader &reader)
    : d(new EwsMailboxPrivate())
{
    while (reader.readNextStartElement()) {
        if (reader.namespaceUri() != ewsTypeNsUri) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Unexpected namespace in mailbox element:") << reader.namespaceUri();
            return;
        }
        const QStringView readerName = reader.name();
        if (readerName == QLatin1StringView("Name")) {
            d->mName = reader.readElementText();
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid mailbox Name element.");
                return;
            }
        } else if (readerName == QLatin1StringView("EmailAddress")) {
            d->mEmail = reader.readElementText();
            if (reader.error() != QXmlStreamReader::NoError) {
                qCWarning(EWSCLI_LOG) << QStringLiteral("Failed to read EWS request - invalid mailbox EmailAddress element.");
                return;
            }
        } else if (readerName == QLatin1StringView("RoutingType") || readerName == QLatin1StringView("MailboxType")
                   || readerName == QLatin1StringView("ItemId")) {
            // Unsupported - ignore
            // qCWarningNC(EWSCLIENT_LOG) << QStringLiteral("Unsupported mailbox element %1").arg(reader.name().toString());
            reader.skipCurrentElement();
        }
    }

    d->mValid = true;
}

EwsMailboxPrivate::~EwsMailboxPrivate()
{
}

EwsMailbox::EwsMailbox(const EwsMailbox &other)
    : d(other.d)
{
}

EwsMailbox::EwsMailbox(EwsMailbox &&other)
    : d(std::move(other.d))
{
}

EwsMailbox::~EwsMailbox()
{
}

EwsMailbox &EwsMailbox::operator=(const EwsMailbox &other)
{
    d = other.d;
    return *this;
}

EwsMailbox &EwsMailbox::operator=(EwsMailbox &&other)
{
    d = std::move(other.d);
    return *this;
}

bool EwsMailbox::isValid() const
{
    return d->mValid;
}

QString EwsMailbox::name() const
{
    return d->mName;
}

QString EwsMailbox::email() const
{
    return d->mEmail;
}

QString EwsMailbox::emailWithName() const
{
    if (d->mName.isEmpty()) {
        return d->mEmail;
    } else {
        return QStringLiteral("%1 <%2>").arg(d->mName, d->mEmail);
    }
}

EwsMailbox::operator KMime::Types::Mailbox() const
{
    KMime::Types::Mailbox mbox;
    mbox.setAddress(d->mEmail.toLatin1());
    if (!d->mName.isEmpty()) {
        mbox.setName(d->mName);
    }
    return mbox;
}
