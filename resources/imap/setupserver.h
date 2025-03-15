/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
   SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
   SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>
   SPDX-FileCopyrightText: 2006-2008 Omat Holding B.V. <info@omat.nl>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "config-kdepim-runtime.h"
#include "settings.h"

#include <Akonadi/Collection>
#include <QDialog>

#include <QRegularExpressionValidator>

#if HAVE_ACTIVITY_SUPPORT
namespace PimCommonActivities
{
class ConfigureActivitiesWidget;
}
#endif

class KJob;
class QComboBox;
namespace Ui
{
class SetupServerView;
}

namespace MailTransport
{
class ServerTest;
}
namespace KIdentityManagementWidgets
{
class IdentityCombo;
}
class FolderArchiveSettingPage;

/**
 * @class SetupServer
 * These contain the account settings
 * @author Tom Albers <tomalbers@kde.nl>
 */
class SetupServer : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param settings The settings
     * @param identifier The identifier of the resource
     * @param parent The parent widget
     */
    explicit SetupServer(Settings &settings, const QString &identifier, QWidget *parent);

    void loadSettings();
    void saveSettings();

    /**
     * Destructor
     */
    ~SetupServer() override;

Q_SIGNALS:
    void okEnabled(bool enabled);

private:
    void slotMailCheckboxChanged();
    void slotEncryptionRadioChanged();
    void slotSubcriptionCheckboxChanged();
    void slotShowServerInfo();
    void passwordFetched();
    void sievePasswordFetched();
    void populateDefaultAuthenticationOptions();

    Settings &m_settings;
    const QString m_identifier;
    Ui::SetupServerView *const m_ui;
    MailTransport::ServerTest *m_serverTest = nullptr;
    bool m_subscriptionsChanged = false;
    bool m_shouldClearCache = false;
    QString m_vacationFileName;
    KIdentityManagementWidgets::IdentityCombo *m_identityCombobox = nullptr;
    QString m_oldResourceName;
    QRegularExpressionValidator mValidator;
    Akonadi::Collection mOldTrash;
    FolderArchiveSettingPage *m_folderArchiveSettingPage = nullptr;
#if HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif

private Q_SLOTS:
    void slotTest();
    void slotFinished(const QList<int> &testResult);
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
