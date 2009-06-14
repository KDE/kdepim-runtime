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

#include <QPushButton>
#include <QLayout>
#include <QTabWidget>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTreeWidget>

#include <akonadi/filter/ui/programeditor.h>
#include <akonadi/filter/ui/componentfactory.h>
#include <akonadi/filter/ui/editorfactory.h>

#include <akonadi/collectiondialog.h>

#include "filter.h"
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
  g->addWidget( mIdLineEdit, 0, 1, 1, 2 );

  mCollectionList = new QTreeWidget( tab );
  g->addWidget( mCollectionList, 1, 0, 1, 3 );

  mAddCollectionButton = new QPushButton( tab );
  mAddCollectionButton->setIcon( KIcon( "edit-new" ) );
  mAddCollectionButton->setText( i18n( "Add Filtered Collection...") );
  g->addWidget( mAddCollectionButton, 2, 0, 1, 2 );

  connect( mAddCollectionButton, SIGNAL( clicked() ), this, SLOT( slotAddCollectionButtonClicked() ) );

  mRemoveCollectionButton = new QPushButton( tab );
  mRemoveCollectionButton->setIcon( KIcon( "edit-delete" ) );
  mRemoveCollectionButton->setText( i18n( "Remove Filtered Collection") );
  g->addWidget( mRemoveCollectionButton, 2, 2 );

  connect( mRemoveCollectionButton, SIGNAL( clicked() ), this, SLOT( slotRemoveCollectionButtonClicked() ) );

  g->setRowStretch( 1, 1 );

  mProgramEditor = new Akonadi::Filter::UI::ProgramEditor( tabWidget, mFilter->componentFactory(), mFilter->editorFactory() );
  tabWidget->addTab( mProgramEditor, i18n( "Program" ) );

}

FilterEditor::~FilterEditor()
{
}

void FilterEditor::slotAddCollectionButtonClicked()
{
  Akonadi::CollectionDialog d( this );
  QStringList mimeTypes;
  mimeTypes << QLatin1String( "message/rfc822" );
  d.setMimeTypeFilter( mimeTypes );
  d.exec();
}

void FilterEditor::slotRemoveCollectionButtonClicked()
{
}

