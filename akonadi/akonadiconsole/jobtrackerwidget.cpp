/*
    This file is part of Akonadi.

    Copyright (c) 2009 Till Adam <adam@kde.org>

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

#include "jobtrackerwidget.h"

#include "jobtrackermodel.h"

#include <akonadi/control.h>

#include <KLocale>

#include <QtGui/QTreeView>
#include <QtGui/QHeaderView>
#include <QtGui/QVBoxLayout>
#include <QMenu>


class JobTrackerWidget::Private
{
  public:
    JobTrackerModel* model;
};

JobTrackerWidget::JobTrackerWidget( const char *name, QWidget *parent )
  : QWidget( parent ),
  d( new Private )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QTreeView *tv = new QTreeView( this );
  d->model = new JobTrackerModel( name, this );
  tv->setModel( d->model );
  tv->expandAll();
  tv->setAlternatingRowColors( true );
  tv->setContextMenuPolicy( Qt::CustomContextMenu );
  tv->header()->setResizeMode( QHeaderView::ResizeToContents );
  connect( d->model, SIGNAL( modelReset( ) ), tv, SLOT( expandAll() ) );
  connect( tv, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)) );
  layout->addWidget( tv );
  d->model->setEnabled( false ); // since it can be slow, default to off

  Akonadi::Control::widgetNeedsAkonadi( this );
}

void JobTrackerWidget::contextMenu( const QPoint &pos )
{
  QMenu menu;
  menu.addAction( i18n( "Clear View" ), d->model, SLOT(resetTracker()) );
  QAction *action = new QAction(&menu);
  action->setCheckable( true );
  action->setChecked( d->model->isEnabled() );
  action->setText( i18n("Enabled") );
  connect( action, SIGNAL(toggled(bool)), d->model, SLOT(setEnabled(bool)) );
  menu.addAction( action );
  menu.exec( mapToGlobal( pos ) );
}


#include "jobtrackerwidget.moc"
