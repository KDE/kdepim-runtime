/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 3 only, as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License version 3 for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "akonaditemplateloader.h"

#include <akonadi/collection.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <akonadi/itemfetchscope.h>
#include "entitytreemodel.h"

#include <QAbstractItemModel>

#include <kdebug.h>

using namespace Akonadi;

AkonadiTemplateLoader::AkonadiTemplateLoader(Akonadi::Monitor *monitor,  QObject* parent )
  : AbstractTemplateLoader( parent ), m_themeName("default")
{
  Collection rootCollection = Collection::root();

  Session *session = new Session( QByteArray( "AkonadiTemplateLoader-" ) + QByteArray::number( qrand() ), this );

  m_model = new EntityTreeModel( session, monitor, this );

}

Grantlee::MutableTemplate* AkonadiTemplateLoader::loadMutableByName( const QString &name ) const
{
  Item templateItem = getItem( name );
  if ( !templateItem.isValid() )
    return 0;

  QString content = templateItem.payloadData();
  MutableTemplate *t = Engine::instance()->newMutableTemplate( content );
  return t;
}

Akonadi::Item AkonadiTemplateLoader::getItem( const QString& name ) const
{
  Akonadi::Item invalidItem;
  QModelIndex idx = m_model->index(0, 0);
  if (!idx.isValid())
    return invalidItem;

  QModelIndex resourceIndex = m_model->index(0,0,idx);
  if (!resourceIndex.isValid())
    return invalidItem;

  QModelIndex themeIndex = m_model->match(resourceIndex, EntityTreeModel::RemoteIdRole, m_themeName, 1 ).value(0);
  if (!themeIndex.isValid())
    return invalidItem;

  QModelIndex firstTemplateIndex = m_model->index(0,0, themeIndex);
  if (!firstTemplateIndex.isValid())
    return invalidItem;

  QModelIndex templateIndex = m_model->match(firstTemplateIndex, EntityTreeModel::RemoteIdRole, name, 1).value(0);
  if (!templateIndex.isValid())
    return invalidItem;

  return templateIndex.data(EntityTreeModel::ItemRole).value<Item>();
}

Grantlee::Template *AkonadiTemplateLoader::loadByName( const QString& name ) const
{
  Item templateItem = getItem( name );
  if ( !templateItem.isValid() )
    return 0;

  QString content = templateItem.payloadData();
  Template *t = Engine::instance()->newTemplate( content );
  return t;
}


