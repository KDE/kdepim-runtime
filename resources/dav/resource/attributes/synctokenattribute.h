/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Akonadi/Attribute>

#include <QString>

class SyncTokenAttribute : public Akonadi::Attribute
{
public:
    explicit SyncTokenAttribute(const QString &syncToken = QString());

    void setSyncToken(const QString &syncToken);
    QString syncToken() const;

    Akonadi::Attribute *clone() const override;
    QByteArray type() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    QString mSyncToken;
};
