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

#include "migrationstatuswidget.h"
#include "migrationscheduler.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QAction>
#include <QListView>
#include <QApplication>
#include <QLabel>
#include <KToolBar>
#include <KDebug>
#include <KIcon>
#include <KDialog>
#include <KLocalizedString>

MigrationStatusWidget::MigrationStatusWidget(MigrationScheduler &scheduler, QWidget *parent)
    :QWidget(parent),
    mScheduler(scheduler)
{
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    {
        KToolBar *toolbar = new KToolBar(QLatin1String("MigrationControlToolbar"), this);

        QAction *start = toolbar->addAction(QLatin1String("Start"));
        start->setIcon(KIcon(QLatin1String("media-playback-start")));
        connect(start, SIGNAL(triggered(bool)), this, SLOT(startSelected()));

        QAction *pause = toolbar->addAction(QLatin1String("Pause"));
        pause->setIcon(KIcon(QLatin1String("media-playback-pause")));
        connect(pause, SIGNAL(triggered(bool)), this, SLOT(pauseSelected()));

        QAction *abort = toolbar->addAction(QLatin1String("Abort"));
        abort->setIcon(KIcon(QLatin1String("media-playback-stop")));
        connect(abort, SIGNAL(triggered(bool)), this, SLOT(abortSelected()));

        vboxLayout->addWidget(toolbar);
    }
    {
        QTreeView *treeView = new QTreeView(this);
        treeView->setModel(&mScheduler.model());
        mSelectionModel = treeView->selectionModel();
        Q_ASSERT(mSelectionModel);
        //Not sure why this is required, but otherwise the view doesn't load anything from the model
        treeView->update(QModelIndex());
        connect(treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onItemActivated(QModelIndex)));

        vboxLayout->addWidget(treeView);
    }
    setLayout(vboxLayout);
}

void MigrationStatusWidget::startSelected()
{
    foreach (const QModelIndex &index, mSelectionModel->selectedRows()) {
        mScheduler.start(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::pauseSelected()
{
    foreach (const QModelIndex &index, mSelectionModel->selectedRows()) {
        mScheduler.pause(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::abortSelected()
{
    foreach (const QModelIndex &index, mSelectionModel->selectedRows()) {
        mScheduler.abort(index.data(MigratorModel::IdentifierRole).toString());
    }
}

void MigrationStatusWidget::onItemActivated(const QModelIndex &index)
{
    KDialog *dlg = new KDialog(this);
    QWidget *widget = new QWidget(dlg);

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
        QLabel *label = new QLabel(QString::fromLatin1("<a href=\"%1\">%2</a>").arg(index.data(MigratorModel::LogfileRole).toString()).arg(ki18n("Logfile").toString()), widget);
        label->setOpenExternalLinks(true);
        hboxLayout->addWidget(label);
        hboxLayout->addStretch();
        vboxLayout->addLayout(hboxLayout);
    }
    widget->setLayout(vboxLayout);
    dlg->setMainWidget(widget);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setCaption(i18nc("Title of the window displaying the log of a single migration job.", "Migration Info"));
    dlg->setButtons(KDialog::Close);
    dlg->resize(600, 300);
    dlg->show();
}


