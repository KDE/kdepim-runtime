/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "notificationmonitor.h"
#include "notificationmodel.h"

#include <Akonadi/Control>

#include <KLocale>

#include <QHeaderView>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

NotificationMonitor::NotificationMonitor(QWidget* parent) :
  QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QTreeView *tv = new QTreeView( this );
  m_model = new NotificationModel( this );
  tv->setModel( m_model );
  tv->expandAll();
  tv->setAlternatingRowColors( true );
  tv->setContextMenuPolicy( Qt::CustomContextMenu );
  tv->header()->setResizeMode( QHeaderView::ResizeToContents );
  connect( tv, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)) );
  layout->addWidget( tv );
  m_model->setEnabled( false ); // since it can be slow, default to off

  Akonadi::Control::widgetNeedsAkonadi( this );
}

void NotificationMonitor::contextMenu(const QPoint& pos)
{
  QMenu menu;
  menu.addAction( i18n( "Clear View" ), m_model, SLOT(clear()) );
  QAction *action = new QAction(&menu);
  action->setCheckable( true );
  action->setChecked( m_model->isEnabled() );
  action->setText( i18n("Enabled") );
  connect( action, SIGNAL(toggled(bool)), m_model, SLOT(setEnabled(bool)) );
  menu.addAction( action );
  menu.exec( mapToGlobal( pos ) );
}

#include "notificationmonitor.moc"
