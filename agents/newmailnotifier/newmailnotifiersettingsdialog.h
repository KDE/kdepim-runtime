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

#ifndef NEWMAILNOTIFIERSETTINGSDIALOG_H
#define NEWMAILNOTIFIERSETTINGSDIALOG_H

#include <KDialog>
#include <Akonadi/Collection>

namespace Akonadi
{
class CollectionModel;
class CollectionView;
class RecursiveCollectionFilterProxyModel;
}

class QModelIndex;
class QItemSelectionModel;
class KNotifyConfigWidget;
class QCheckBox;
class KLineEdit;
class KJob;

class NewMailNotifierSettingsDialog : public KDialog
{
    Q_OBJECT
public:
    explicit NewMailNotifierSettingsDialog(QWidget *parent=0);
    ~NewMailNotifierSettingsDialog();

private Q_SLOTS:
    void slotOkClicked();
    void slotHelpLinkClicked(const QString &);

    void slotCollectionsInserted(const QModelIndex &parent, int start, int end);

    void slotSelectAllCollections();
    void slotUnselectAllCollections();

    void slotModifyJobDone(KJob *job);

    void setCollectionFilter(const QString &filter);

private:
    void updateCollectionsRecursive(const QModelIndex &parent);
    void selectAllCollectionsRecursive(const QModelIndex &parent, bool select);

    QCheckBox *mShowPhoto;
    QCheckBox *mShowFrom;
    QCheckBox *mShowSubject;
    QCheckBox *mShowFolders;
    QCheckBox *mExcludeMySelf;
    KNotifyConfigWidget *mNotify;
    QCheckBox *mTextToSpeak;
    KLineEdit *mTextToSpeakSetting;
    QItemSelectionModel *mSelectionModel;
    Akonadi::CollectionModel *mCollectionModel;
    Akonadi::CollectionView *mCollectionView;
    Akonadi::RecursiveCollectionFilterProxyModel *mCollectionFilter;

};

#endif // NEWMAILNOTIFIERSETTINGSDIALOG_H
