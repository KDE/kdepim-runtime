/*
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright 2009 Thomas McGuire <mcguire@kde.org>
 *   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef ACCOUNT_WIDGET_H
#define ACCOUNT_WIDGET_H

#include "ui_popsettings.h"

namespace MailTransport {
class ServerTest;
}
namespace KWallet {
class Wallet;
}

class KJob;

class AccountWidget : public QWidget, private Ui::PopPage
{
    Q_OBJECT

public:
    AccountWidget(const QString &identifier, QWidget *parent);
    ~AccountWidget() override;

    void loadSettings();
    void saveSettings();

Q_SIGNALS:
    void okEnabled(bool enabled);

private Q_SLOTS:
    void slotEnablePopInterval(bool state);
    void slotLeaveOnServerClicked();
    void slotEnableLeaveOnServerDays(bool state);
    void slotEnableLeaveOnServerCount(bool state);
    void slotEnableLeaveOnServerSize(bool state);
    void slotFilterOnServerClicked();
    void slotPipeliningClicked();
    void slotPopEncryptionChanged(QAbstractButton *button);
    void slotCheckPopCapabilities();
    void slotPopCapabilities(const QVector<int> &);
    void slotLeaveOnServerDaysChanged(int value);
    void slotLeaveOnServerCountChanged(int value);
    void slotFilterOnServerSizeChanged(int value);

    void targetCollectionReceived(Akonadi::Collection::List collections);
    void localFolderRequestJobFinished(KJob *job);
    void walletOpenedForLoading(bool success);
    void walletOpenedForSaving(bool success);
    void slotAccepted();
private:
    void setupWidgets();
    void checkHighest(QButtonGroup *);
    void enablePopFeatures();
    void populateDefaultAuthenticationOptions();

private:
    QButtonGroup *encryptionButtonGroup = nullptr;
    MailTransport::ServerTest *mServerTest = nullptr;
    QRegExpValidator mValidator;
    bool mServerTestFailed = false;
    KWallet::Wallet *mWallet = nullptr;
    QString mInitalPassword;
    QString mIdentifier;
};

#endif
