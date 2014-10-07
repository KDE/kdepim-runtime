/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "typepage.h"

#include <QDebug>
#include <KDesktopFile>
#include <KConfigGroup>
#include <QIcon>

#include <QSortFilterProxyModel>
#include <QMimeDatabase>
#include <QMimeType>
#include "global.h"
#include <QStandardPaths>
#include <QDir>

TypePage::TypePage(KAssistantDialog *parent) :
    Page(parent),
    m_model(new QStandardItemModel(this))
{
    ui.setupUi(this);
#ifdef KDEPIM_MOBILE_UI
    ui.label->setHidden(true);
    ui.searchLine->setHidden(true);
#endif

#ifdef ACCOUNTWIZARD_NO_GHNS
    ui.ghnsButton->hide();
#endif

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_model);
    ui.listView->setModel(proxy);
    ui.searchLine->setProxy(proxy);

 
    QStringList list;
    QStringList files;

    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/accountwizard/"), QStandardPaths::LocateDirectory);
    Q_FOREACH (const QString &dir, dirs) {
        const QStringList directories = QDir(dir).entryList(QDir::AllDirs);
        Q_FOREACH (const QString &directory, directories) {
           const QString fullPath = dir + QLatin1Char('/') + directory;
           const QStringList fileNames = QDir(fullPath).entryList(QStringList() << QStringLiteral("*.desktop"));
           Q_FOREACH (const QString &file, fileNames) {
              list.append(fullPath + QLatin1Char('/') + file);

           }
        }
    }

    const QStringList filter = Global::typeFilter();
    foreach (const QString &entry, list) {
        KDesktopFile f(entry);
        qDebug() << entry << f.readName();
        const KConfig configWizard(entry);
        KConfigGroup grp(&configWizard, "Wizard");
        const QStringList lstType = grp.readEntry("Type", QStringList());
        if (lstType.isEmpty()) {
            qDebug() << QString::fromLatin1(" %1 doesn't contains specific type").arg(f.readName());
        }
        if (!filter.isEmpty()) {
            // stolen from agentfilterproxymodel
            bool found = false;
            foreach (const QString &mimeType, lstType) {
                if (filter.contains(mimeType)) {
                    found = true;
                    break;
                } else {
                    foreach (const QString &type, filter) {
                        QMimeDatabase db;
                        QMimeType typePtr = db.mimeTypeForName(type);
                        if (typePtr.isValid() && typePtr.inherits(mimeType)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (found) {
                    break;
                }
            }
            if (!found) {
                continue;
            }
        }
        QStandardItem *item = new QStandardItem(f.readName());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(entry, Qt::UserRole);
        if (!f.readIcon().isEmpty()) {
            item->setData(QIcon::fromTheme(f.readIcon()), Qt::DecorationRole);
        }
        item->setData(f.readComment(), Qt::ToolTipRole);
        m_model->appendRow(item);
    }

    connect(ui.listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged()));
    connect(ui.ghnsButton, &QPushButton::clicked, this, &TypePage::ghnsWanted);
}

void TypePage::selectionChanged()
{
    if (ui.listView->selectionModel()->hasSelection()) {
        setValid(true);
    } else {
        setValid(false);
    }
}

void TypePage::leavePageNext()
{
    if (!ui.listView->selectionModel()->hasSelection()) {
        return;
    }
    const QModelIndex index = ui.listView->selectionModel()->selectedIndexes().first();
    Global::setAssistant(index.data(Qt::UserRole).toString());
}

QTreeView *TypePage::treeview() const
{
    return ui.listView;
}

