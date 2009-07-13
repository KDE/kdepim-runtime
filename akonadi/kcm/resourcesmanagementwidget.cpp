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
#include <akonadi/agenttypedialog.h>
#include <akonadi/control.h>

#include <kwindowsystem.h>

#include <KDebug>
#include <KMenu>
#include <KMessageBox>
#include <KMimeType>

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

    d->ui.resourcesList->agentFilterProxyModel()->addCapabilityFilter( "Resource" );
    foreach( const QString& type, d->wantedMimeTypes )
        d->ui.resourcesList->agentFilterProxyModel()->addMimeTypeFilter( type );
    connect( d->ui.resourcesList, SIGNAL( currentChanged( const Akonadi::AgentInstance&, const Akonadi::AgentInstance& ) ),
             SLOT( updateButtonState( const Akonadi::AgentInstance& ) ) );
    connect( d->ui.resourcesList, SIGNAL( doubleClicked( const Akonadi::AgentInstance& ) ),
             SLOT( editClicked() ) );

    connect( d->ui.addButton, SIGNAL( clicked () ), SLOT( addClicked() ) );
    connect( d->ui.editButton, SIGNAL( clicked() ), SLOT( editClicked() ) );
    connect( d->ui.removeButton, SIGNAL( clicked() ), SLOT( removeClicked() ) );

    updateButtonState( d->ui.resourcesList->currentAgentInstance() );

    Akonadi::Control::widgetNeedsAkonadi( this );
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
        d->ui.editButton->setEnabled( !current.type().capabilities().contains( QLatin1String( "NoConfig" ) ) );
        d->ui.removeButton->setEnabled( true );
    }
}

void ResourcesManagementWidget::addClicked()
{
    Akonadi::AgentTypeDialog dlg( this );
    Akonadi::AgentFilterProxyModel* filter = dlg.agentFilterProxyModel();
    foreach( const QString& type, d->wantedMimeTypes )
        filter->addMimeTypeFilter( type );

    if ( dlg.exec() ) {
        const Akonadi::AgentType agentType = dlg.agentType();

        if ( agentType.isValid() ) {

            Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( agentType, this );
            job->configure( this );
            job->start();
        }
    }
}

void ResourcesManagementWidget::editClicked()
{
    Akonadi::AgentInstance instance = d->ui.resourcesList->currentAgentInstance();
    if ( instance.isValid() ) {
        KWindowSystem::allowExternalProcessWindowActivation();
        instance.configure( this );
    }
}

void ResourcesManagementWidget::removeClicked()
{
    const Akonadi::AgentInstance instance = d->ui.resourcesList->currentAgentInstance();
    if ( instance.isValid() )
        Akonadi::AgentManager::self()->removeInstance( instance );

    updateButtonState( d->ui.resourcesList->currentAgentInstance() );
}

#include "resourcesmanagementwidget.moc"
