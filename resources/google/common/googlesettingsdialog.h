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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLESETTINGSDIALOG_H
#define GOOGLESETTINGSDIALOG_H

#include <KDE/KDialog>

#include <LibKGAPI2/Types>

namespace KGAPI2
{
class Job;
}

class GoogleResource;
class GoogleSettings;
class GoogleAccountManager;

class QGroupBox;
class KComboBox;
class QCheckBox;
class KIntSpinBox;


class GoogleSettingsDialog : public KDialog
{
    Q_OBJECT

  public:
    explicit GoogleSettingsDialog( GoogleAccountManager *accountManager, WId wId, GoogleResource *parent );
    virtual ~GoogleSettingsDialog();

    GoogleAccountManager* accountManager() const;
    KGAPI2::AccountPtr currentAccount() const;

  public Q_SLOTS:
    void reloadAccounts();

  Q_SIGNALS:
    void currentAccountChanged( const QString &accountName );

  protected:
    bool handleError( KGAPI2::Job *job );
    virtual void saveSettings() = 0;

  private Q_SLOTS:
    void slotSaveSettings();
    void slotAddAccountClicked();
    void slotRemoveAccountClicked();
    void slotAuthJobFinished( KGAPI2::Job *job );
    void slotAccountAuthenticated( KGAPI2::Job *job );

  private:
    GoogleResource *m_parentResource;
    GoogleAccountManager *m_accountManager;

    QGroupBox *m_accGroupBox;
    KPushButton *m_addAccButton;
    KPushButton *m_removeAccButton;
    KComboBox *m_accComboBox;
    QCheckBox *m_enableRefresh;
    KIntSpinBox *m_refreshSpinBox;

};

#endif // GOOGLESETTINGSDIALOG_H

