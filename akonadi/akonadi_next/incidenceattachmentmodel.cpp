/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "incidenceattachmentmodel.h"

#include <akonadi/entitytreemodel.h>
#include <Akonadi/Monitor>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemFetchJob>

using namespace Akonadi;

namespace Akonadi
{

class IncidenceAttachmentModelPrivate
{
  IncidenceAttachmentModelPrivate( IncidenceAttachmentModel *qq, const QPersistentModelIndex &modelIndex, Akonadi::Item item = Akonadi::Item() )
    : q_ptr( qq ), m_modelIndex( modelIndex ), m_item( item ), m_monitor( 0 )
  {
    if ( modelIndex.isValid() ) {
      QObject::connect( modelIndex.model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), qq, SLOT(resetModel()) );
    } else if ( item.isValid() ) {
      createMonitor();
      resetInternalData();
    }
    QHash<int, QByteArray> roleNames = qq->roleNames();
    roleNames.insert( IncidenceAttachmentModel::MimeTypeRole, "mimeType" );
    roleNames.insert( IncidenceAttachmentModel::AttachmentUrl, "attachmentUrl" );
    qq->setRoleNames( roleNames );
  }

  void resetModel()
  {
    Q_Q( IncidenceAttachmentModel );
    q->beginResetModel();
    resetInternalData();
    q->endResetModel();
    emit q->rowCountChanged();
  }

  void itemFetched( Akonadi::Item::List list )
  {
    Q_ASSERT( list.size() == 1 );
    setItem( list.first() );
  }

  void setItem( const Akonadi::Item &item );

  void createMonitor()
  {
    if ( m_monitor )
      return;

    m_monitor = new Akonadi::Monitor( q_ptr );
    m_monitor->setItemMonitored( m_item );
    m_monitor->itemFetchScope().fetchFullPayload( true );
    QObject::connect( m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), q_ptr, SLOT(resetModel()) );
    QObject::connect( m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), q_ptr, SLOT(resetModel()) );
  }

  void resetInternalData()
  {
    if ( m_incidence )
      m_incidence->clearTempFiles();
    Item item = m_item;
    if ( m_modelIndex.isValid() )
      item = m_modelIndex.data( EntityTreeModel::ItemRole ).value<Akonadi::Item>();

    if ( !item.isValid() || !item.hasPayload<KCal::Incidence::Ptr>() )
    {
      m_incidence.reset();
      return;
    }
    m_incidence = item.payload<KCal::Incidence::Ptr>();
  }

  Q_DECLARE_PUBLIC( IncidenceAttachmentModel )
  IncidenceAttachmentModel * const q_ptr;

  QModelIndex m_modelIndex;
  Akonadi::Item m_item;
  KCal::Incidence::Ptr m_incidence;
  Akonadi::Monitor *m_monitor;
};

}

IncidenceAttachmentModel::IncidenceAttachmentModel( const QPersistentModelIndex &modelIndex, QObject* parent )
  : QAbstractListModel( parent ), d_ptr( new IncidenceAttachmentModelPrivate( this, modelIndex ) )
{

}

IncidenceAttachmentModel::IncidenceAttachmentModel( const Akonadi::Item &item, QObject* parent )
  : QAbstractListModel( parent ), d_ptr( new IncidenceAttachmentModelPrivate( this, QModelIndex(), item ) )
{

}

IncidenceAttachmentModel::IncidenceAttachmentModel( QObject* parent )
  : QAbstractListModel( parent ), d_ptr( new IncidenceAttachmentModelPrivate( this, QModelIndex() ) )
{

}

KCal::Incidence::Ptr IncidenceAttachmentModel::incidence() const
{
  Q_D( const IncidenceAttachmentModel );
  return d->m_incidence;
}

void IncidenceAttachmentModel::setIndex( const QPersistentModelIndex &modelIndex )
{
  Q_D( IncidenceAttachmentModel );
  beginResetModel();
  d->m_modelIndex = modelIndex;
  d->m_item = Akonadi::Item();
  d->resetInternalData();
  endResetModel();
  emit rowCountChanged();
}

void IncidenceAttachmentModel::setItem( const Akonadi::Item& item )
{
  Q_D( IncidenceAttachmentModel );
  if ( !item.hasPayload<KCal::Incidence::Ptr>() )
  {
    ItemFetchJob *job = new ItemFetchJob( item );
    job->fetchScope().fetchFullPayload( true );
    connect( job, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemFetched(Akonadi::Item::List)) );
    return;
  }
  d->setItem( item );
}

void IncidenceAttachmentModelPrivate::setItem( const Akonadi::Item& item )
{
  Q_Q( IncidenceAttachmentModel );
  q->beginResetModel();
  m_modelIndex = QModelIndex();
  m_item = item;
  createMonitor();
  resetInternalData();
  q->endResetModel();
  emit q->rowCountChanged();
}

int IncidenceAttachmentModel::rowCount( const QModelIndex& parent ) const
{
  Q_D( const IncidenceAttachmentModel );
  if ( !d->m_incidence )
    return 0;
  return d->m_incidence->attachments().size();
}

QVariant IncidenceAttachmentModel::data(const QModelIndex& index, int role) const
{
  Q_D( const IncidenceAttachmentModel );
  if ( !d->m_incidence )
    return QVariant();

  KCal::Attachment *attachment = d->m_incidence->attachments().at( index.row() );
  switch ( role )
  {
    case Qt::DisplayRole:
      return attachment->label();
    case AttachmentDataRole:
      return attachment->decodedData();
    case MimeTypeRole:
      return attachment->mimeType();
    case AttachmentUrl:
      return d->m_incidence->writeAttachmentToTempFile( attachment );
  }
  return QVariant();
}

QVariant IncidenceAttachmentModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  return QAbstractItemModel::headerData(section, orientation, role);
}

#include "incidenceattachmentmodel.moc"
