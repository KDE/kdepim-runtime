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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "migrationstatuswidget.h"
#include "migrationscheduler.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QAction>
#include <QListView>
#include <QLabel>
#include <QToolBar>
#include <QIcon>
#include <QDialog>
#include <KLocalizedString>
#include <QDialogButtonBox>

MigrationStatusWidget::MigrationStatusWidget(MigrationScheduler &scheduler, QWidget *parent)
    : QWidget(parent)
    , mScheduler(scheduler)
{
    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    QToolBar *toolbar = new QToolBar(QStringLiteral("MigrationControlToolbar"), this);

    QAction *start = toolbar->addAction(QStringLiteral("Start"));
    start->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    connect(start, &QAction::triggered, this, &MigrationStatusWidget::startSelected);

    QAction *pause = toolbar->addAction(QStringLiteral("Pause"));
    pause->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    connect(pause, &QAction::triggered, this, &MigrationStatusWidget::pauseSelected);

    QAction *abort = toolbar->addAction(QStringLiteral("Abort"));
    abort->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
    connect(abort, &QAction::triggered, this, &MigrationStatusWidget::abortSelected);

    vboxLayout->addWidget(toolbar);
    QTreeView *treeView = new QTreeView(this);
    treeView->setModel(&mScheduler.model());
    mSelectionModel = treeView->selectionModel();
    Q_ASSERT(mSelectionModel);
    //Not sure why this is required, but otherwise the view doesn't load anything from the model
    treeView->update(QModelIndex());
    connect(treeView, &QTreeView::doubleClicked, this, &MigrationStatusWidget::onItemActivated);

    vboxLayout->addWidget(treeView);
}

void MigrationStatusWidget::startSelected()
{
    const QModelIndexList lst = mSelectionModel->selectedRows();
    for (const QModelIndex &index : lst) {
        mScheduler.start(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::pauseSelected()
{
    const QModelIndexList lst = mSelectionModel->selectedRows();
    for (const QModelIndex &index : lst) {
        mScheduler.pause(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::abortSelected()
{
    const QModelIndexList lst = mSelectionModel->selectedRows();
    for (const QModelIndex &index : lst) {
        mScheduler.abort(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::onItemActivated(const QModelIndex &index)
{
    QDialog *dlg = new QDialog(this);
    QVBoxLayout *topLayout = new QVBoxLayout(dlg);
    dlg->setLayout(topLayout);
    QWidget *widget = new QWidget(dlg);
    topLayout->addWidget(widget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    topLayout->addWidget(buttonBox);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    {
        QListView *listView = new QListView(widget);
        listView->setModel(&mScheduler.logModel(index.data(MigratorModel::IdentifierRole).toString()));
        listView->setAutoScroll(true);
        listView->scrollToBottom();
        vboxLayout->addWidget(listView);
    }
    {
        QHBoxLayout *hboxLayout = new QHBoxLayout;
        QLabel *label = new QLabel(QStringLiteral("<a href=\"%1\">%2</a>").arg(index.data(MigratorModel::LogfileRole).toString()).arg(ki18n("Logfile").toString()), widget);
        label->setOpenExternalLinks(true);
        hboxLayout->addWidget(label);
        hboxLayout->addStretch();
        vboxLayout->addLayout(hboxLayout);
    }
    widget->setLayout(vboxLayout);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(i18nc("Title of the window displaying the log of a single migration job.", "Migration Info"));
    dlg->resize(600, 300);
    dlg->show();
}
