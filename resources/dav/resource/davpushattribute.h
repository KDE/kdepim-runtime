/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>
#include <QDateTime>

#include <QString>
#include <QUrl>

class DavPushAttribute : public Akonadi::Attribute
{
public:
    explicit DavPushAttribute();

    [[nodiscard]] QString topic() const;
    void setTopic(const QString &topic);

    [[nodiscard]] QUrl registrationUrl() const;
    void setRegistrationUrl(const QUrl &registrationUrl);

    [[nodiscard]] QDateTime expirationDate() const;
    void setExpirationDate(const QDateTime &expirationDate);

    [[nodiscard]] Akonadi::Attribute *clone() const override;
    [[nodiscard]] QByteArray type() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QString mTopic;
    QUrl mRegistrationUrl;
    QDateTime mExpirationDate;
};
