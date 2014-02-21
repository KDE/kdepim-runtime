/*
    Copyright (c) 2013 Laurent Montel <montel@kde.org>

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

#ifndef NEWMAILNOTIFIERSELECTCOLLECTIONWIDGET_H
#define NEWMAILNOTIFIERSELECTCOLLECTIONWIDGET_H

#include <QWidget>
#include <Akonadi/Collection>
#include <QModelIndex>

class QItemSelectionModel;
class KRecursiveFilterProxyModel;
namespace Akonadi {
class EntityTreeModel;
class ChangeRecorder;
}
class QTreeView;
class KCheckableProxyModel;
class KJob;

class NewMailNotifierSelectCollectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewMailNotifierSelectCollectionWidget(QWidget *parent=0);
    ~NewMailNotifierSelectCollectionWidget();

    void updateCollectionsRecursive(const QModelIndex &parent);

private Q_SLOTS:
    void slotSelectAllCollections();
    void slotUnselectAllCollections();
    void slotModifyJobDone(KJob* job);
    void slotUpdateCollectionStatus();
    void slotSetCollectionFilter(const QString&);

    void slotCollectionTreeFetched();

private:
    void updateStatus(const QModelIndex &parent);
    void forceStatus(const QModelIndex &parent, bool status);
    QTreeView *mFolderView;
    QItemSelectionModel *mSelectionModel;
    Akonadi::EntityTreeModel *mModel;
    Akonadi::ChangeRecorder *mChangeRecorder;
    KCheckableProxyModel *mCheckProxy;
    KRecursiveFilterProxyModel *mCollectionFilter;
    bool mNeedUpdate;
};

#endif // NEWMAILNOTIFIERSELECTCOLLECTIONWIDGET_H
