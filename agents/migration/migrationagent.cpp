/*
 * SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */
#include "migrationagent.h"

#include "migrationstatuswidget.h"

#include "migration/gid/gidmigrator.h"
#include "migration/googlegroupware/googleresourcemigrator.h"
#include "migration/icalcategoriestotags/icalcategoriestotagsmigrator.h"

#include <KContacts/Addressee>
#include <KLocalizedString>
#include <KWindowSystem>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Akonadi
{
MigrationAgent::MigrationAgent(const QString &id)
    : AgentWidgetBase(id)
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("akonadi_migration_agent"));
    mScheduler.addMigrator(QSharedPointer<GidMigrator>::create(KContacts::Addressee::mimeType()));
    mScheduler.addMigrator(QSharedPointer<GoogleResourceMigrator>::create());
    mScheduler.addMigrator(QSharedPointer<ICalCategoriesToTagsMigrator>::create());
}

void MigrationAgent::configure(WId windowId)
{
    auto dlg = new QDialog();
    auto topLayout = new QVBoxLayout(dlg);

    auto widget = new MigrationStatusWidget(mScheduler, dlg);
    topLayout->addWidget(widget);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    topLayout->addWidget(buttonBox);

    dlg->setWindowTitle(i18nc("Title of the window that shows the status of the migration agent and offers controls to start/stop individual migration jobs.",
                              "Migration Status"));
    dlg->resize(600, 300);

    if (windowId) {
        dlg->setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(dlg->windowHandle(), windowId);
    }
    dlg->show();
}
}

AKONADI_AGENT_MAIN(Akonadi::MigrationAgent)

#include "moc_migrationagent.cpp"
