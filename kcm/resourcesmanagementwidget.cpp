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

#include "resourcesmanagementwidget.h"
#include "ui_resourcesmanagementwidget.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentfilterproxymodel.h>

#include <KDebug>
#include <KMenu>
#include <KMessageBox>
#include <KMimeType>

static bool isWantedResource( const QStringList &wantedMimeTypes, const QStringList &resourceMimeTypes )
{
  foreach ( const QString &resourceMimeType, resourceMimeTypes ) {
    KMimeType::Ptr mimeType = KMimeType::mimeType( resourceMimeType, KMimeType::ResolveAliases );
    if ( mimeType.isNull() )
      continue;

    // check if the resource MIME type is or inherits from the
    // wanted one, e.g.
    // if the resource offers message/news and wanted types includes
    // message/rfc822 this will be true, because message/news is a
    // "subclass" of message/rfc822
    foreach ( const QString &wantedMimeType, wantedMimeTypes ) {
      if ( mimeType->is( wantedMimeType ) )
        return true;
    }
  }

  return false;
}

class ResourcesManagementWidget::Private
{
public:
    Ui::ResourcesManagementWidget   ui;
    QHash<QAction*, Akonadi::AgentType>        menuOptions;
    QStringList                     wantedMimeTypes;
};

ResourcesManagementWidget::ResourcesManagementWidget( QWidget *parent,  const QStringList &args ) :
        QWidget( parent ),
        d( new Private )
{
    d->wantedMimeTypes = args;
    d->ui.setupUi( this );

    KMenu *addMenu = new KMenu( this );
    bool atleastone = false;

    Akonadi::AgentType::List agentTypes = Akonadi::AgentManager::self()->types();
    foreach( const Akonadi::AgentType &agentType, agentTypes ) {

        if ( !agentType.capabilities().contains( "Resource" ) )
            continue;

        const QStringList mimeTypes = agentType.mimeTypes();
        bool wanted = isWantedResource( d->wantedMimeTypes, mimeTypes );

        if ( !wanted && !d->wantedMimeTypes.isEmpty() )
            continue;

        QAction *action = addMenu->addAction( agentType.name() );
        action->setIcon( agentType.icon() );
        d->menuOptions.insert( action, agentType );
        atleastone = true;
    }

    if ( !atleastone ) {
        QAction *action = addMenu->addAction( i18n( "Akonadi server is not running or "
                                              "the server could not find any resources." ) );
        action->setEnabled( false );
        KMessageBox::information( this, i18n( "Akonadi server is not running or "
                                              "the server could not find any resources." ), i18n( "No resources found" ) );
    }

    d->ui.addButton->setMenu( addMenu );
    connect( addMenu, SIGNAL( triggered( QAction* ) ), SLOT( addClicked( QAction* ) ) );

    foreach( const QString& type, d->wantedMimeTypes )
        d->ui.resourcesList->agentFilterProxyModel()->addMimeTypeFilter( type );
    connect( d->ui.resourcesList, SIGNAL( currentChanged( const Akonadi::AgentInstance&, const Akonadi::AgentInstance& ) ),
             SLOT( updateButtonState( const Akonadi::AgentInstance& ) ) );
    connect( d->ui.resourcesList, SIGNAL( doubleClicked( const Akonadi::AgentInstance& ) ),
             SLOT( editClicked() ) );

    connect( d->ui.editButton, SIGNAL( clicked() ), SLOT( editClicked() ) );
    connect( d->ui.removeButton, SIGNAL( clicked() ), SLOT( removeClicked() ) );

    updateButtonState();
}

ResourcesManagementWidget::~ResourcesManagementWidget()
{
    delete d;
}

void ResourcesManagementWidget::updateButtonState( const Akonadi::AgentInstance& current)
{
    if ( !current.isValid() ) {
        d->ui.editButton->setEnabled( false );
        d->ui.removeButton->setEnabled( false );
    } else {
        d->ui.editButton->setEnabled( true );
        d->ui.removeButton->setEnabled( true );
    }
}

void ResourcesManagementWidget::addClicked( QAction *action )
{
    Q_ASSERT( d->menuOptions.contains( action ) );
    Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( d->menuOptions.value( action ) , this );
    job->configure( this );
    job->start();
}

void ResourcesManagementWidget::editClicked()
{
    Akonadi::AgentInstance instance = d->ui.resourcesList->currentAgentInstance();
    if ( instance.isValid() )
        instance.configure( this );
}

void ResourcesManagementWidget::removeClicked()
{
    const Akonadi::AgentInstance instance = d->ui.resourcesList->currentAgentInstance();
    if ( instance.isValid() )
        Akonadi::AgentManager::self()->removeInstance( instance );

    updateButtonState( d->ui.resourcesList->currentAgentInstance() );
}

#include "resourcesmanagementwidget.moc"
