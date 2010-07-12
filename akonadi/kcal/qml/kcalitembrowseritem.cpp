/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "kcalitembrowseritem.h"

#include <akonadi/item.h>

using namespace Akonadi;
using namespace Akonadi::KCal;

ExtendedIncidenceViewer::ExtendedIncidenceViewer( QWidget *parent )
  : IncidenceViewer( parent )
{
}

void ExtendedIncidenceViewer::itemRemoved()
{
  emit incidenceRemoved();
}


KCalItemBrowserItem::KCalItemBrowserItem(QDeclarativeItem* parent) : DeclarativeAkonadiItem(parent)
{
  m_viewer = new ExtendedIncidenceViewer( 0 );
  connect( m_viewer, SIGNAL( incidenceRemoved() ), SIGNAL( incidenceRemoved() ) );

  setWidget( m_viewer );
}

Akonadi::Item KCalItemBrowserItem::item() const
{
  return m_viewer->item();
}

qint64 KCalItemBrowserItem::itemId() const
{
  return m_viewer->item().id();
}

void KCalItemBrowserItem::setItemId(qint64 id)
{
  m_viewer->setItem( Akonadi::Item( id ) );
  emit attachmentModelChanged();
}

QDate KCalItemBrowserItem::activeDate() const
{
  return m_viewer->activeDate();
}

void KCalItemBrowserItem::setActiveDate( const QDate &date )
{
  m_viewer->setIncidence( m_viewer->item(), date );
}

QObject* KCalItemBrowserItem::attachmentModel() const
{
  return m_viewer->attachmentModel();
}

#include "kcalitembrowseritem.moc"
