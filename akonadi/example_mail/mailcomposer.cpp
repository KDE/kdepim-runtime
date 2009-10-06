/*
 * This file is part of the Akonadi Mail example.
 *
 * Copyright 2009  Stephen Kelly <steveire@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mailcomposer.h"

#include <QGridLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>

#include <QTreeView>
#include <QListView>
#include "amazingcompleter.h"
#include "amazingdelegate.h"
#include <akonadi/entitytreemodel.h>

#include <akonadi/changerecorder.h>
#include <akonadi/session.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <kdescendantsproxymodel.h>
#include <akonadi/entityfilterproxymodel.h>

#include "contactsmodel.h"

using namespace Akonadi;

MailComposer::MailComposer(Akonadi::Session *session, QWidget *parent)
{
  QGridLayout *layout = new QGridLayout();

  QLabel *toLabel = new QLabel(this);
  toLabel->setText( QLatin1String( "To:" ) );

  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
  //   scope.fetchAttribute< CollectionChildOrderAttribute >();
  scope.fetchAttribute< EntityDisplayAttribute >();

  ChangeRecorder *monitor = new ChangeRecorder( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( QLatin1String( "text/directory" ) );

  ContactsModel *contactsModel = new ContactsModel( session, monitor, this);

  KDescendantsProxyModel *descProxyModel = new KDescendantsProxyModel(this);
  descProxyModel->setSourceModel(contactsModel);
//  descProxyModel->setHeaderSet(EntityTreeModel::ItemListHeaders);

  EntityFilterProxyModel *filter = new EntityFilterProxyModel(this);
  filter->setSourceModel(descProxyModel);
  filter->addMimeTypeExclusionFilter(Collection::mimeType());

  QLineEdit *emailLineEdit = new QLineEdit(this);

  QLabel *subjectLabel = new QLabel(this);
  subjectLabel->setText( QLatin1String( "Subject:" ) );
  QLineEdit *subjectLineEdit = new QLineEdit(this);

  QTextEdit *textEdit = new QTextEdit(this);

  QListView *popup = new QListView(this);

  QListView *amazingView = new QListView(this);
  popup->setItemDelegate(new AmazingContactItemDelegate(this));

  layout->addWidget(toLabel, 0, 0);
  layout->addWidget(emailLineEdit, 0, 1);
  layout->addWidget(subjectLabel, 1, 0);
  layout->addWidget(subjectLineEdit, 1, 1);
  layout->addWidget(textEdit, 2, 0, 1, 2);
  layout->addWidget(amazingView, 3, 0, 1, 2);

  this->setLayout(layout);

  AmazingCompleter *completer = new AmazingCompleter(this);
  completer->setModel(filter);
  completer->setMatchingRole(Akonadi::EntityTreeModel::AmazingCompletionRole);
  completer->setWidget(emailLineEdit);

  connect(emailLineEdit, SIGNAL(textChanged(QString)), completer, SLOT(setCompletionPrefixString(QString)));

  completer->setView(popup, AmazingCompleter::Popup);

}



