/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davpushattribute.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

DavPushAttribute::DavPushAttribute() = default;

QString DavPushAttribute::topic() const
{
    return mTopic;
}

void DavPushAttribute::setTopic(const QString &topic)
{
    mTopic = topic;
}

QUrl DavPushAttribute::registrationUrl() const
{
    return mRegistrationUrl;
}

void DavPushAttribute::setRegistrationUrl(const QUrl &registrationUrl)
{
    mRegistrationUrl = registrationUrl;
}

QDateTime DavPushAttribute::expirationDate() const
{
    return mExpirationDate;
}

void DavPushAttribute::setExpirationDate(const QDateTime &expirationDate)
{
    mExpirationDate = expirationDate;
}

Akonadi::Attribute *DavPushAttribute::clone() const
{
    auto res = new DavPushAttribute();
    res->mTopic = this->mTopic;
    res->mRegistrationUrl = this->mRegistrationUrl;
    res->mExpirationDate = this->mExpirationDate;
    return res;
}

QByteArray DavPushAttribute::type() const
{
    static const QByteArray sType("davpush");
    return sType;
}

QByteArray DavPushAttribute::serialized() const
{
    auto res = QByteArray();
    auto out = QDataStream(&res, QIODevice::WriteOnly);
    out << mTopic << mRegistrationUrl << mExpirationDate;
    return res;
}

void DavPushAttribute::deserialize(const QByteArray &data)
{
    auto in = QDataStream(data);
    in >> mTopic >> mRegistrationUrl >> mExpirationDate;
}
