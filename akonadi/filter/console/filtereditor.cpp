/******************************************************************************
 *
 *  File : filtereditor.cpp
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "filtereditor.h"

#include <KDebug>
#include <KMessageBox>
#include <KLocale>
#include <KIcon>

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QShowEvent>
#include <QtGui/QTabWidget>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

#include <akonadi/collection.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/entitydisplayattribute.h>

#include <akonadi/filter/agent.h>
#include <akonadi/filter/componentfactory.h>
#include <akonadi/filter/program.h>

#include <akonadi/filter/ui/programeditor.h>
#include <akonadi/filter/ui/editorfactory.h>

#include "filter.h"
#include "filtercollectionmodel.h"
#include "mainwindow.h"



FilterEditor::FilterEditor( QWidget * parent, Filter * filter, bool lbb )
  : KDialog( parent ), mFilter( filter )
{
  setCaption( i18n( "Filter Editor" ) );

  QTabWidget * tabWidget = new QTabWidget( this );
  setMainWidget( tabWidget );

  QWidget * tab = new QWidget( tabWidget );
  tabWidget->addTab( tab, i18n( "General" ) );

  QGridLayout * g = new QGridLayout( tab );

  QLabel * l = new QLabel( i18n( "Id" ), tab );
  g->addWidget( l, 0, 0 );

  mIdLineEdit = new QLineEdit( tab );

  QString id = filter->id();
  if( id.isEmpty() )
  {
    // generate an unique id
    id = QString::fromAscii( "my_filter_%1" ).arg( QDateTime::currentDateTime().toTime_t() );
  }

  mIdLineEdit->setText( id );

  mIdLineEdit->setToolTip(
      i18n(
          "The unique identifier of this filtering program.<br>" \
          "It can be any string, it's enough that it's globally unique.<br>" \
          "In user applications this will be usually automatically assigned and hidden."
        )
    );

  g->addWidget( mIdLineEdit, 0, 1, 1, 2 );

  l = new QLabel( i18n( "Name" ), tab );
  g->addWidget( l, 1, 0 );

  mNameLineEdit = new QLineEdit( tab );
  mNameLineEdit->setText( filter->program()->name() );

  mNameLineEdit->setToolTip(
      i18n( "The user-visible name of this filtering program: can be any string." )
    );

  g->addWidget( mNameLineEdit, 1, 1, 1, 2 );

  mFilterCollectionModel = new FilterCollectionModel( this, filter );

  mCollectionList = new QTreeView( tab );
  g->addWidget( mCollectionList, 2, 0, 1, 3 );
  mCollectionList->setModel( mFilterCollectionModel );

  g->setRowStretch( 2, 1 );

  // Our mighty program editor :)
  mProgramEditor = new Akonadi::Filter::UI::ProgramEditor(
      tabWidget,
      mFilter->componentFactory(),
      mFilter->editorFactory(),
      lbb ? Akonadi::Filter::UI::ProgramEditor::ListBoxBased : Akonadi::Filter::UI::ProgramEditor::ToolBoxBased
    );

  tabWidget->addTab( mProgramEditor, i18n( "Program" ) );

  mProgramEditor->fillFromProgram( filter->program() );

  setMinimumSize( 640, 480 );
}

FilterEditor::~FilterEditor()
{
}

void FilterEditor::done( int result )
{
  if( result != KDialog::Accepted )
  {
    KDialog::done( result );
    return;
  }

  QString id = mIdLineEdit->text().trimmed();
  if( id.isEmpty() )
  {
    KMessageBox::error( this, i18n( "The filter id can't be empty" ), i18n( "Can't commit filter" ) );
    return;
  }

  Akonadi::Filter::Program * prog = mProgramEditor->commit();
  if( !prog )
    return;

  prog->setName( mNameLineEdit->text().trimmed() );

  mFilter->setProgram( prog );

  mFilter->setId( id );  

  KDialog::done( result );
}

void FilterEditor::expandCollections( const QModelIndex &parentIdx )
{
  if( parentIdx.isValid() )
    mCollectionList->setExpanded( parentIdx, true );
  int cnt = mFilterCollectionModel->rowCount( parentIdx );
  for( int i = 0; i < cnt; i++ )
  {
    QModelIndex idx = mFilterCollectionModel->index( i, 0, parentIdx );
    Q_ASSERT( idx.isValid() );
    expandCollections( idx );
  }
}

void FilterEditor::showEvent( QShowEvent *e )
{
  KDialog::showEvent( e );

  // Bleah :/
  QTimer::singleShot( 500, this, SLOT( autoExpandCollections() ) );
}

void FilterEditor::autoExpandCollections()
{
  expandCollections( QModelIndex() );
}

