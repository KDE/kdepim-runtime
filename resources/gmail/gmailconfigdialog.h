/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILCONFIGDIALOG_H
#define GMAILCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class GmailConfigDialog;
}

namespace KIdentityManagement {
class IdentityCombo;
class IdentityManager;
}
class QPushButton;
class GmailResource;
class FolderArchiveSettingPage;
class GmailConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GmailConfigDialog(GmailResource *resource, WId parent);
    ~GmailConfigDialog() override;

    bool shouldClearCache() const;

private Q_SLOTS:
    /**
     * Call this if you want the settings saved from this page.
     */
    void applySettings();
    void slotIdentityCheckboxChanged();
    void slotMailCheckboxChanged();
    void slotManageSubscriptions();
    void slotSubcriptionCheckboxChanged();
    void slotComplete();

    void slotAuthenticate();
    void onAccountRequestCompleted(const KGAPI2::AccountPtr &account, bool userRejected);

private:
    void readSettings();

    GmailResource *m_parentResource = nullptr;
    Ui::GmailConfigDialog *m_ui = nullptr;
    bool m_subscriptionsChanged;
    bool m_shouldClearCache;
    KIdentityManagement::IdentityManager *m_identityManager = nullptr;
    KIdentityManagement::IdentityCombo *m_identityCombobox = nullptr;
    QString m_oldResourceName;
    KGAPI2::AccountPtr m_account;
    FolderArchiveSettingPage *m_folderArchiveSettingPage = nullptr;
    QPushButton *mOkButton = nullptr;
};

#endif // GMAILCONFIGDIALOG_H
