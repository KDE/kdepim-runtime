/*
    Copyright (c) 2013-2018 Laurent Montel <montel@kde.org>

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
#include <Collection>
#include <QModelIndex>
#include <QIdentityProxyModel>
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
class KRecursiveFilterProxyModel;
#else
class QSortFilterProxyModel;
#endif
namespace Akonadi {
class EntityTreeModel;
class ChangeRecorder;
}
class QTreeView;
class KJob;

class NewMailNotifierCollectionProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    explicit NewMailNotifierCollectionProxyModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &_data, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<Akonadi::Collection, bool> notificationCollection() const;

private:
    QHash<Akonadi::Collection, bool> mNotificationCollection;
};

class NewMailNotifierSelectCollectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewMailNotifierSelectCollectionWidget(QWidget *parent = nullptr);
    ~NewMailNotifierSelectCollectionWidget();

    void updateCollectionsRecursive();

private:
    void slotSelectAllCollections();
    void slotUnselectAllCollections();
    void slotModifyJobDone(KJob *job);
    void slotSetCollectionFilter(const QString &);

    void slotCollectionTreeFetched();
    void forceStatus(const QModelIndex &parent, bool status);
    QTreeView *mFolderView = nullptr;
    Akonadi::EntityTreeModel *mModel = nullptr;
    Akonadi::ChangeRecorder *mChangeRecorder = nullptr;
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
    KRecursiveFilterProxyModel *mCollectionFilter = nullptr;
#else
    QSortFilterProxyModel *mCollectionFilter = nullptr;
#endif
    NewMailNotifierCollectionProxyModel *mNewMailNotifierProxyModel = nullptr;
};

#endif // NEWMAILNOTIFIERSELECTCOLLECTIONWIDGET_H
