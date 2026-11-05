// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "singlefileresourceconfigquickbase.h"

class MBoxConfigQuick : public SingleFileResourceConfigQuickBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the resource configuration plugin")
    Q_PROPERTY(int compactFrequency READ compactFrequency WRITE setCompactFrequency NOTIFY mboxSettingsChanged)
    Q_PROPERTY(int messageCount READ messageCount WRITE setMessageCount NOTIFY mboxSettingsChanged)
    Q_PROPERTY(int lockfileMethod READ lockfileMethod WRITE setLockfileMethod NOTIFY mboxSettingsChanged)
    Q_PROPERTY(QString lockfile READ lockfile WRITE setLockfile NOTIFY mboxSettingsChanged)
    Q_PROPERTY(bool procmailAvailable READ procmailAvailable CONSTANT)
    Q_PROPERTY(bool muttAvailable READ muttAvailable CONSTANT)
public:
    MBoxConfigQuick(const KSharedConfigPtr &config, const QString &identifier, QObject *parent);
    int compactFrequency() const;
    void setCompactFrequency(int value);
    int messageCount() const;
    void setMessageCount(int value);
    int lockfileMethod() const;
    void setLockfileMethod(int value);
    QString lockfile() const;
    void setLockfile(const QString &value);
    bool procmailAvailable() const;
    bool muttAvailable() const;
    void load() override;
Q_SIGNALS:
    void mboxSettingsChanged();

private:
    const bool mProcmailAvailable;
    const bool mMuttAvailable;
};
