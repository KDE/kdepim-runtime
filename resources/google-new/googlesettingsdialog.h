/*
   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>
                 2020 Igor Poboiko <igor.poboiko@gmail.com>

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

namespace Ui {
    class GoogleSettingsDialog;
}
namespace KGAPI2 {
    class Job;
}
class GoogleResource;

class GoogleSettingsDialog : public QDialog 
{
    Q_OBJECT
public:
    explicit GoogleSettingsDialog(GoogleResource *resource, WId wId);
    ~GoogleSettingsDialog();
protected:
    bool handleError(KGAPI2::Job *job);
    void accountChanged();
private:
    GoogleResource *m_resource = nullptr;
    Ui::GoogleSettingsDialog *m_ui = nullptr;
    KGAPI2::AccountPtr m_account;
private Q_SLOTS:
    void slotConfigure();
    void slotAuthJobFinished(KGAPI2::Job *job);
    void slotSaveSettings();
    void slotReloadCalendars();
    void slotReloadTaskLists();
};

#endif // GOOGLESETTINGSDIALOG_H
