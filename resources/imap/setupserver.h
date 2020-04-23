/* This file is part of the KDE project
   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
   Copyright (C) 2009 Kevin Ottens <ervin@kde.org>
   Copyright (C) 2006-2008 Omat Holding B.V. <info@omat.nl>
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef SETUPSERVER_H
#define SETUPSERVER_H

#include <QDialog>
#include <collection.h>
#include <KJob>

#include <QRegExpValidator>
class QPushButton;
class QComboBox;
namespace Ui {
class SetupServerView;
}

namespace MailTransport {
class ServerTest;
}
namespace KIdentityManagement {
class IdentityCombo;
}
class FolderArchiveSettingPage;
class ImapResourceBase;

/**
 * @class SetupServer
 * These contain the account settings
 * @author Tom Albers <tomalbers@kde.nl>
 */
class SetupServer : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parentResource The resource this dialog belongs to
     * @param parent Parent WId
     */
    SetupServer(ImapResourceBase *parentResource, WId parent);

    /**
     * Destructor
     */
    ~SetupServer();

    bool shouldClearCache() const;

private Q_SLOTS:
    /**
     * Call this if you want the settings saved from this page.
     */
    void applySettings();
    void slotIdentityCheckboxChanged();
    void slotMailCheckboxChanged();
    void slotEncryptionRadioChanged();
    void slotSubcriptionCheckboxChanged();
    void slotShowServerInfo();
private:
    void readSettings();
    void populateDefaultAuthenticationOptions();

    ImapResourceBase *m_parentResource = nullptr;
    Ui::SetupServerView *m_ui = nullptr;
    MailTransport::ServerTest *m_serverTest = nullptr;
    bool m_subscriptionsChanged = false;
    bool m_shouldClearCache = false;
    QString m_vacationFileName;
    KIdentityManagement::IdentityCombo *m_identityCombobox = nullptr;
    QString m_oldResourceName;
    QRegExpValidator mValidator;
    Akonadi::Collection mOldTrash;
    FolderArchiveSettingPage *m_folderArchiveSettingPage = nullptr;
    QPushButton *mOkButton = nullptr;

private Q_SLOTS:
    void slotTest();
    void slotFinished(const QVector<int> &testResult);
    void slotCustomSieveChanged();

    void slotServerChanged();
    void slotTestChanged();
    void slotComplete();
    void slotSafetyChanged();
    void slotManageSubscriptions();
    void slotEnableWidgets();
    void targetCollectionReceived(const Akonadi::Collection::List &collections);
    void localFolderRequestJobFinished(KJob *job);
    void populateDefaultAuthenticationOptions(QComboBox *combo);
};

#endif
