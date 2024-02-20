/*
    SPDX-FileCopyrightText: 2017-2018 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#ifdef EWSSETTINGS_UNITTEST
#include "ewssettings_ut_mock.h"
#else
#include "ewssettingsbase.h"
#endif

class EwsAbstractAuth;

class EwsSettings : public EwsSettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Ews.Wallet")
public:
    explicit EwsSettings(QObject *parent = nullptr);
    ~EwsSettings() override;

    /// Request the password from Qt KeyChain
    /// Emit passwordRequestFinished when finished
    void requestPassword();

    /// Request a map of oauth tokens from Qt KeyChain
    /// Emit mapRequestFinished when finished
    void requestMap();

    EwsAbstractAuth *loadAuth(QObject *parent);

public Q_SLOTS:
    /// Set the new password
    Q_SCRIPTABLE void setPassword(const QString &password);

    /// Set the map of oauth token
    Q_SCRIPTABLE void setMap(const QMap<QString, QString> &map);

    /// Set password for unit tests
    Q_SCRIPTABLE void setTestPassword(const QString &password);

Q_SIGNALS:
    void passwordRequestFinished(const QString &password);
    void mapRequestFinished(const QMap<QString, QString> &map);

private:
    QString mPassword;
};
