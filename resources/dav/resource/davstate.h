/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <ksharedconfig.h>

class QByteArray;
class QString;

class DavState : public QObject
{
    Q_OBJECT

public:
    explicit DavState(const KSharedConfigPtr &config);

    [[nodiscard]] QString getToken() const;
    void setToken(const QString &token);
    void clearToken();

    [[nodiscard]] QByteArray getVapidPublicKey() const;
    void setVapidPublicKey(const QByteArray &vapidPublicKey);
    void clearVapidPublicKey();

private:
    KSharedConfigPtr mConfig;
};
