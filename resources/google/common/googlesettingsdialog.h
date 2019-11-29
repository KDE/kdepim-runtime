/*
   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GOOGLESETTINGSDIALOG_H
#define GOOGLESETTINGSDIALOG_H

#include <QDialog>

#include <KGAPI/Types>

namespace KGAPI2 {
class Job;
}

class GoogleResource;
class GoogleSettings;
class GoogleAccountManager;

class QGroupBox;
class QComboBox;
class QCheckBox;
class KPluralHandlingSpinBox;
class QPushButton;
class QVBoxLayout;

class GoogleSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoogleSettingsDialog(GoogleAccountManager *accountManager, WId wId, GoogleResource *parent);
    ~GoogleSettingsDialog() override;

    GoogleAccountManager *accountManager() const;
    KGAPI2::AccountPtr currentAccount() const;

public Q_SLOTS:
    void reloadAccounts();

Q_SIGNALS:
    void currentAccountChanged(const QString &accountName);

protected:
    bool handleError(KGAPI2::Job *job);
    virtual void saveSettings() = 0;
    QVBoxLayout *mainLayout() const;

private Q_SLOTS:
    void slotSaveSettings();
    void slotAddAccountClicked();
    void slotRemoveAccountClicked();
    void slotAuthJobFinished(KGAPI2::Job *job);
    void slotAccountAuthenticated(KGAPI2::Job *job);

private:
    GoogleResource *m_parentResource = nullptr;
    GoogleAccountManager *m_accountManager = nullptr;

    QGroupBox *m_accGroupBox = nullptr;
    QPushButton *m_addAccButton = nullptr;
    QPushButton *m_removeAccButton = nullptr;
    QComboBox *m_accComboBox = nullptr;
    QCheckBox *m_enableRefresh = nullptr;
    KPluralHandlingSpinBox *m_refreshSpinBox = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
};

#endif // GOOGLESETTINGSDIALOG_H
