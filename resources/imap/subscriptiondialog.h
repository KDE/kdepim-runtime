/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef SUBSCRIPTIONDIALOG_H
#define SUBSCRIPTIONDIALOG_H

#include <KDE/KDialog>

#include <krecursivefilterproxymodel.h>
#include <kimap/listjob.h>

#include <QtCore/QMap>

class QKeyEvent;
class QStandardItemModel;
class QStandardItem;

class KDescendantsProxyModel;
class KLineEdit;
class QCheckBox;
class ImapAccount;
class QTreeView;
class QListView;


class SubscriptionFilterProxyModel : public KRecursiveFilterProxyModel
{
  Q_OBJECT
public:
  explicit SubscriptionFilterProxyModel( QObject* parent = 0 );

public slots:
  void setSearchPattern( const QString &pattern );
  void setIncludeCheckedOnly( bool checkedOnly );
  void setIncludeCheckedOnly( int checkedOnlyState );

protected:
  /*reimp*/ bool acceptRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
  QString m_pattern;
  bool m_checkedOnly;
};


class SubscriptionDialog : public KDialog
{
  Q_OBJECT
public:
  enum Roles {
    InitialStateRole = Qt::UserRole + 1,
    PathRole
  };
  enum SubscriptionDialogOption {
    None = 0,
    AllowToEnableSubscription = 1
  };
  Q_DECLARE_FLAGS( SubscriptionDialogOptions, SubscriptionDialogOption )

  explicit SubscriptionDialog( QWidget *parent = 0, SubscriptionDialog::SubscriptionDialogOptions option = SubscriptionDialog::None );
  ~SubscriptionDialog();

  void connectAccount( const ImapAccount &account, const QString &password );
  bool isSubscriptionChanged() const;
  void setSubscriptionEnabled( bool enabled );
  bool subscriptionEnabled() const;

private slots:
  void onLoginDone( KJob *job );
  void onReloadRequested();
  void onMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
                            const QList< QList<QByteArray> > &flags );
  void onFullListingDone( KJob *job );
  void onSubscribedMailBoxesReceived( const QList<KIMAP::MailBoxDescriptor> &mailBoxes,
                                      const QList< QList<QByteArray> > &flags );
  void onReloadDone( KJob *job );
  void onItemChanged( QStandardItem *item );
  void onMobileLineEditChanged( const QString &text );

protected:
  /* reimp */ void keyPressEvent( QKeyEvent *event );

protected slots:
  void slotButtonClicked( int button );
private:
  void applyChanges();

  KIMAP::Session *m_session;
  bool m_subscriptionChanged;

#ifndef KDEPIM_MOBILE_UI
  QTreeView *m_treeView;
#else
  QListView* m_listView;
#endif

  KLineEdit *m_lineEdit;
  QCheckBox *m_enableSubscription;
  SubscriptionFilterProxyModel *m_filter;
  KDescendantsProxyModel *m_flatModel;
  QStandardItemModel *m_model;
  QMap<QString, QStandardItem*> m_itemsMap;
};

#endif
