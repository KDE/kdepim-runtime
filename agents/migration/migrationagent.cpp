/*
 * Copyright 2013  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "migrationagent.h"

#include "migrationstatuswidget.h"

#include <migration/gid/gidmigrator.h>
#include <KABC/Addressee>
#include <KWindowSystem>
#include <KDialog>
#include <KLocalizedString>
#include <kuiserverjobtracker.h>

namespace Akonadi {

MigrationAgent::MigrationAgent(const QString &id)
:   AgentBase(id),
    mScheduler(new KUiServerJobTracker)
{
    mScheduler.addMigrator(QSharedPointer<GidMigrator>(new GidMigrator(KABC::Addressee::mimeType())));
}

void MigrationAgent::configure(WId windowId)
{
    KDialog *dlg = new KDialog();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setMainWidget(new MigrationStatusWidget(mScheduler, dlg));
    dlg->setCaption(i18nc("Title of the window that shows the status of the migration agent and offers controls to start/stop individual migration jobs.", "Migration Status"));
    dlg->setButtons(KDialog::Close);
    dlg->resize(600, 300);

    if (windowId) {
    #ifndef Q_WS_WIN
        KWindowSystem::setMainWindow(dlg, windowId);
    #else
        KWindowSystem::setMainWindow(dlg, (HWND)windowId);
    #endif
    }
    dlg->show();
}

}

AKONADI_AGENT_MAIN(Akonadi::MigrationAgent)

