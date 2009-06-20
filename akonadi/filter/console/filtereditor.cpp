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

#include <QLayout>
#include <QTabWidget>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTreeView>

#include <akonadi/filter/agent.h>

#include <akonadi/filter/ui/programeditor.h>
#include <akonadi/filter/ui/componentfactory.h>
#include <akonadi/filter/ui/editorfactory.h>

#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/collectionmodel.h>

#include "filter.h"
#include "filtercollectionmodel.h"
#include "mainwindow.h"



FilterEditor::FilterEditor( QWidget * parent, Filter * filter )
  : KDialog( parent ), mFilter( filter )
{
  setCaption( i18n( "Filter Editor" ) );

  QTabWidget * tabWidget = new QTabWidget( this );
  setMainWidget( tabWidget );

  QWidget * tab = new QWidget( tabWidget );
  tabWidget->addTab( tab, i18n( "General" ) );

  QGridLayout * g = new QGridLayout( tab );

  QLabel * l = new QLabel( i18n( "Filter Id" ), tab );
  g->addWidget( l, 0, 0 );

  mIdLineEdit = new QLineEdit( tab );
  mIdLineEdit->setText( filter->id() );
  g->addWidget( mIdLineEdit, 0, 1, 1, 2 );

  mFilterCollectionModel = new FilterCollectionModel( this, filter );

  mCollectionList = new QTreeView( tab );
  g->addWidget( mCollectionList, 1, 0, 1, 3 );
  mCollectionList->setModel( mFilterCollectionModel );

  g->setRowStretch( 1, 1 );

  mProgramEditor = new Akonadi::Filter::UI::ProgramEditor( tabWidget, mFilter->componentFactory(), mFilter->editorFactory() );
  tabWidget->addTab( mProgramEditor, i18n( "Program" ) );
  mProgramEditor->fillFromProgram( filter->program() );
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

  mFilter->setProgram( prog );

  mFilter->setId( id );  

  KDialog::done( result );
}

