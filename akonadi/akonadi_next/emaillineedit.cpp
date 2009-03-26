/*
    This file is part of the Akonadi Mail example.

    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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


#include "emaillineedit.h"

#include <QCompleter>
#include <QDirModel>
#include <QTreeView>

#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>

#include "entityupdateadapter.h"
#include "descendantentitiesproxymodel.h"
#include "entityfilterproxymodel.h"

#include "contactsmodel.h"

#include <kdebug.h>

using namespace Akonadi;

EmailLineEdit::EmailLineEdit(Akonadi::Session *session, QWidget *parent)
  : QLineEdit(parent)
//   : KComboBox(parent)
{
//     this->setEditable(true);
    ItemFetchScope scope;
    scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
  //   scope.fetchAttribute< CollectionChildOrderAttribute >();
    scope.fetchAttribute< EntityDisplayAttribute >();

    Monitor *monitor = new Monitor( this );
    monitor->fetchCollection( true );
    monitor->setItemFetchScope( scope );
    monitor->setCollectionMonitored( Collection::root() );

//     Session *session = new Session( QByteArray( "ContactsApplication-" ) + QByteArray::number( qrand() ), this );
//     EntityUpdateAdapter *eua = new EntityUpdateAdapter( session, this );

    ContactsModel *contactsModel = new ContactsModel( session, monitor, this);
    contactsModel->fetchMimeTypes( QStringList() << "text/directory" );

    DescendantEntitiesProxyModel *descProxy = new DescendantEntitiesProxyModel(this);
    descProxy->setSourceModel(contactsModel);

    EntityFilterProxyModel *filterProxy = new EntityFilterProxyModel(this);
    filterProxy->setSourceModel(descProxy);

    filterProxy->addMimeTypeExclusionFilter( Collection::mimeType() );

    QCompleter *completer = new QCompleter(filterProxy, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionColumn(2);
//     completer->setCompletionRole(ContactsModel::EmailCompletionRole);

    this->setCompleter(completer);
//     this->setModel(filterProxy);


}
