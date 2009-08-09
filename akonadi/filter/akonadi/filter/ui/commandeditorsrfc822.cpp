/****************************************************************************** * *
 *
 *  File : commandeditorsrfc822.cpp
 *  Created on Mon 03 Aug 2009 00:46:12 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include <akonadi/filter/ui/commandeditorsrfc822.h>

#include "../action.h"
#include "../commanddescriptor.h"

#include "widgethighlighter.h"

#include <akonadi/filter/componentfactoryrfc822.h>

#include <akonadi/collectionrequester.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>

namespace Akonadi
{
namespace Filter
{
namespace UI
{

CommandWithTargetCollectionEditor::CommandWithTargetCollectionEditor(
    QWidget * parent,
    const CommandDescriptor * commandDescriptor,
    ComponentFactory * componentFactory,
    EditorFactory * editorFactory
  )
  : CommandEditor( parent, commandDescriptor, componentFactory, editorFactory )
{
  QGridLayout * g = new QGridLayout( this );

  QLabel * l = new QLabel( this );
  l->setText( i18n( "Target folder:" ) );

  g->addWidget( l, 0, 0 );

  mCollectionRequester = new CollectionRequester( this );

  QStringList mimes;
  mimes << "message/rfc822";

  mCollectionRequester->setMimeTypeFilter( mimes );

  g->addWidget( mCollectionRequester, 0, 1 );

  g->setColumnStretch( 1, 1 );
}

CommandWithTargetCollectionEditor::~CommandWithTargetCollectionEditor()
{
}

void CommandWithTargetCollectionEditor::fillFromAction( Action::Base * action )
{
  Q_ASSERT( action );
  Q_ASSERT( action->actionType() == Action::ActionTypeCommand );

  Action::Command * cmd = dynamic_cast< Action::Command * >( action );
  Q_ASSERT( cmd );
  Q_ASSERT( cmd->parameters()->count() >= 1 ); // tollerate broken sieve scripts which have more than one param here

  const CommandDescriptor * cmdDescriptor = cmd->commandDescriptor();
  Q_ASSERT( cmdDescriptor );
  Q_ASSERT( cmdDescriptor->parameters()->count() == 1 );

  const CommandDescriptor::ParameterDescriptor * formalParam = cmdDescriptor->parameters()->first();
  Q_ASSERT( formalParam );

  Q_ASSERT( formalParam->dataType() == DataTypeInteger );

  QVariant v = cmd->parameters()->first();

  bool ok;
  qlonglong id = v.toLongLong( &ok );
  if( !ok )
  {
    kWarning() << "The target collection parameter '" << v << "' of the action is not convertible to a Collection::Id";
    return;
  }

  // Must fetch the collection to have its name displayed...

  Akonadi::CollectionFetchJob * job = new Akonadi::CollectionFetchJob( Akonadi::Collection( id ), Akonadi::CollectionFetchJob::Base );

  if( !job->exec() )
  {
    kWarning() << "The target collection with id '" << id << "' is not valid!";
    return;    
  }

  // The Akonadi::Job docs say that the job deletes itself...

  Akonadi::Collection::List collections = job->collections();

  Q_ASSERT( collections.count() == 1 );

  Akonadi::Collection collection = collections[0];

  mCollectionRequester->setCollection( collection );

}

Action::Base * CommandWithTargetCollectionEditor::commitState( Component * parent )
{
  QList< QVariant > params;

  Collection col = mCollectionRequester->collection();
  if( !col.isValid() )
  {
    KMessageBox::sorry( this, i18n( "You must select a target folder for the message" ), i18n( "Invalid target folder" ) );
    new Private::WidgetHighlighter( this );
    return 0;
  }

  Akonadi::CollectionFetchJob * job = new Akonadi::CollectionFetchJob( col, Akonadi::CollectionFetchJob::Base );

  if( !job->exec() )
  {
    KMessageBox::sorry( this, i18n( "Could not find the specified target folder\r\n%1", job->errorString() ), i18n( "Invalid target folder" ) );
    new Private::WidgetHighlighter( this );
    return 0;    
  }

  // The Akonadi::Job docs say that the job deletes itself...

  params.append( QVariant( static_cast< qlonglong >( col.id() ) ) );

  Action::Command * cmd = componentFactory()->createCommand( parent, commandDescriptor(), params );
  if( !cmd )
  {
    KMessageBox::sorry( this, componentFactory()->lastError(), i18n( "Failed to commit the action" ) );
    new Private::WidgetHighlighter( this );
    return 0;
  }

  return cmd;
}







CommandWithStringParamEditor::CommandWithStringParamEditor(
    QWidget * parent,
    const CommandDescriptor * commandDescriptor,
    ComponentFactory * componentFactory,
    EditorFactory * editorFactory
  )
  : CommandEditor( parent, commandDescriptor, componentFactory, editorFactory )
{
  QGridLayout * g = new QGridLayout( this );

  const CommandDescriptor::ParameterDescriptor * param = commandDescriptor->parameters()->first();
  Q_ASSERT( param );

  QLabel * l = new QLabel( this );
  l->setText( QString::fromAscii( "%1:" ).arg( param->name() ) );

  g->addWidget( l, 0, 0 );

  mParameterLineEdit = new QLineEdit( this );

  g->addWidget( mParameterLineEdit, 0, 1 );

  g->setColumnStretch( 1, 1 );
}

CommandWithStringParamEditor::~CommandWithStringParamEditor()
{
}

void CommandWithStringParamEditor::fillFromAction( Action::Base * action )
{
  Q_ASSERT( action );
  Q_ASSERT( action->actionType() == Action::ActionTypeCommand );

  Action::Command * cmd = dynamic_cast< Action::Command * >( action );
  Q_ASSERT( cmd );
  Q_ASSERT( cmd->parameters()->count() >= 1 ); // tollerate broken sieve scripts which have more than one param here

  const CommandDescriptor * cmdDescriptor = cmd->commandDescriptor();
  Q_ASSERT( cmdDescriptor );
  Q_ASSERT( cmdDescriptor->parameters()->count() == 1 );

  const CommandDescriptor::ParameterDescriptor * formalParam = cmdDescriptor->parameters()->first();
  Q_ASSERT( formalParam );

  Q_ASSERT( formalParam->dataType() == DataTypeString );

  QString val = cmd->parameters()->first().toString();

  mParameterLineEdit->setText( val );
}

Action::Base * CommandWithStringParamEditor::commitState( Component * parent )
{
  QList< QVariant > params;

  params.append( mParameterLineEdit->text() );

  Action::Command * cmd = componentFactory()->createCommand( parent, commandDescriptor(), params );
  if( !cmd )
  {
    KMessageBox::sorry( this, componentFactory()->lastError(), i18n( "Failed to commit the action" ) );
    new Private::WidgetHighlighter( this );
    return 0;
  }

  return cmd;
}




} // namespace UI

} // namespace Filter

} // namespace Akonadi

