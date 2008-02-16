/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <tomalbers@kde.nl>

    Based on KMail code by:
    Copyright (C) 2001-2003 Marc Mutz <mutz@kde.org>

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

#include "imaplibmanagementwidget.h"
#include "ui_imaplibmanagementwidget.h"

#include <libakonadi/agentmanager.h>
#include <libakonadi/agentinstancecreatejob.h>

#include <KDebug>
#include <KMenu>

class ImaplibManagementWidget::Private
{
  public:
    Ui::ImaplibManagementWidget ui;
    Akonadi::AgentManager* manager;
    QHash<QAction*, QString> menuOptions;
};

ImaplibManagementWidget::ImaplibManagementWidget(QWidget * parent) :
    QWidget( parent ),
    d( new Private )
{
  d->ui.setupUi( this );
  d->manager = new Akonadi::AgentManager( this );

  KMenu *addMenu = new KMenu( this );
  QStringList types = d->manager->agentTypes();
  foreach (const QString& type, types) {
        QStringList mimeTypes = d->manager->agentMimeTypes( type );
        if ( !mimeTypes.contains("message/rfc822") )
            continue;

        QAction* action = addMenu->addAction( d->manager->agentName( type ) );
        action->setIcon( d->manager->agentIcon( type ) );
        d->menuOptions.insert( action, type );
  }
  d->ui.addButton->setMenu(addMenu);
  connect( addMenu, SIGNAL( triggered( QAction*) ), SLOT(addClicked(QAction*)));

  d->ui.imaplibList->setHeaderLabels( QStringList() << i18n("Name") );
  connect( d->ui.imaplibList, SIGNAL(currentItemChanged(QTreeWidgetItem*,
           QTreeWidgetItem*)), SLOT(updateButtonState()) );
  connect( d->ui.imaplibList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
           SLOT(editClicked()) );
  connect( d->ui.editButton, SIGNAL(clicked()), SLOT(editClicked()) );
  connect( d->ui.removeButton, SIGNAL(clicked()), SLOT(removeClicked()) );

  connect( d->manager, SIGNAL(  agentInstanceAdded (const QString&) ),
           SLOT(fillImaplibList()) ); 
  connect( d->manager, SIGNAL(  agentInstanceRemoved (const QString&) ),
           SLOT(fillImaplibList()) ); 
  connect( d->manager, SIGNAL( agentInstanceNameChanged( const QString&, const QString& ) ),
           SLOT(fillImaplibList()) ); 
  fillImaplibList();
  updateButtonState();
}

ImaplibManagementWidget::~ImaplibManagementWidget()
{
  delete d;
}

void ImaplibManagementWidget::fillImaplibList()
{
    QStringList instances = d->manager->agentInstances();
    d->ui.imaplibList->clear();
    foreach (const QString& instance, instances) {
        QStringList mimeTypes = d->manager->agentMimeTypes( d->manager->agentInstanceType( instance ) );
        if ( !mimeTypes.contains("message/rfc822") )
            continue;

        QTreeWidgetItem *item = new QTreeWidgetItem( d->ui.imaplibList );
        QString name = d->manager->agentInstanceName( instance );
        if (name.isEmpty()) {
            name = instance;
        }
        item->setData( 0, Qt::UserRole, instance ); 
        item->setText( 0, name ); 
    }
}

void ImaplibManagementWidget::updateButtonState()
{
  if ( !d->ui.imaplibList->currentItem() ) {
    d->ui.editButton->setEnabled( false );
    d->ui.removeButton->setEnabled( false );
  } else {
    d->ui.editButton->setEnabled( true );
    d->ui.removeButton->setEnabled( true );
  }
}

void ImaplibManagementWidget::addClicked( QAction* action)
{
    Q_ASSERT( d->menuOptions.contains( action ) );
    Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( d->menuOptions.value( action) , this );
    job->configure( winId() );
    job->start();
}

void ImaplibManagementWidget::editClicked()
{
   Q_ASSERT( d->ui.imaplibList->currentItem() );
   QString current = d->ui.imaplibList->currentItem()->data( 0, Qt::UserRole ).toString();
   d->manager->agentInstanceConfigure( current, winId() );
}

void ImaplibManagementWidget::removeClicked()
{
   Q_ASSERT( d->ui.imaplibList->currentItem() );
   QString current = d->ui.imaplibList->currentItem()->data( 0, Qt::UserRole ).toString();
   d->manager->removeAgentInstance( current );
}

#include "imaplibmanagementwidget.moc"
