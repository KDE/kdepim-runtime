/******************************************************************************
 *
 * This file is part of libkdepim.
 *
 * Copyright (C) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#include "treewidget.h"

#include <KMenu>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QHeaderView>

namespace KPIM {

TreeWidget::TreeWidget( QWidget * parent, const char * name )
: QTreeWidget( parent )
{
  setObjectName( name );
  setColumnCount( 0 );

  header()->setMovable( true );

  setManualColumnHidingEnabled( true );
}

bool TreeWidget::saveLayout( KConfigGroup &group , const char * keyName ) const
{
  if( !keyName )
    return false;

  group.writeEntry( keyName , QVariant( header()->saveState() ) );

  return true;
}

bool TreeWidget::saveLayout( KConfig * config , const char * groupName , const char * keyName ) const
{
  if( !config || !groupName || !keyName )
    return false;

  KConfigGroup group( config , groupName );
  return saveLayout( group , keyName );
}

bool TreeWidget::restoreLayout( KConfigGroup &group , const char * keyName )
{
  if( !keyName )
    return false;

  // Restore the view column order, width and visibility
  QByteArray state = group.readEntry(
                             keyName , QVariant( QVariant::ByteArray )
                         ).toByteArray();

  if( state.isEmpty() )
    return false;

  // Qt writes also some options that can be set only
  // programmatically. This can drive you crazy if you
  // first setup the view and then restore the layout:
  // it will overwrite your previous settings...

  bool sectionsWereClickable = header()->isClickable();
  bool sectionsWereMovable = header()->isMovable();
  //bool lastSectionWasStretched = header()->stretchLastSection();
  bool sortIndicatorWasShown = header()->isSortIndicatorShown();
  Qt::Alignment originalDefaultAlignment = header()->defaultAlignment();
  //int defaultSectionSize = header()->defaultSectionSize();
  int minimumSectionSize = header()->minimumSectionSize();
  bool cascadingSectionResizes = header()->cascadingSectionResizes();

  if ( !header()->restoreState( state ) )
    return false;


  header()->setClickable( sectionsWereClickable );
  header()->setMovable( sectionsWereMovable );
  header()->setSortIndicatorShown( sortIndicatorWasShown );
  header()->setDefaultAlignment( originalDefaultAlignment );
  header()->setMinimumSectionSize( minimumSectionSize );
  header()->setCascadingSectionResizes( cascadingSectionResizes );
  // FIXME: This would cause the sections to be resized and thus
  //        can't be reliably reset after the configuration
  //        has been read. Can do nothing about that except warning
  //        the user in the docs.
  //header()->setDefaultSectionSize( defaultSectionSize );
  //header()->setStretchLastSection( lastSectionWasStretched ); 

  return true;
}

bool TreeWidget::restoreLayout( KConfig * config , const char * groupName , const char * keyName )
{
  if( !config || !groupName || !keyName )
    return false;

  if ( !config->hasGroup( groupName ) )
    return false;

  KConfigGroup group( config , groupName );
  return restoreLayout( group , keyName );
}

void TreeWidget::setManualColumnHidingEnabled( bool enable )
{
  if( enable )
  {
     header()->setContextMenuPolicy( Qt::CustomContextMenu ); // make sure it's true
     QObject::connect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
                       this, SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
  } else {
     if ( mEnableManualColumnHiding )
         QObject::disconnect( header(), SIGNAL( customContextMenuRequested( const QPoint& ) ),
                              this, SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
  }

  mEnableManualColumnHiding = enable;
}

void TreeWidget::setColumnHidden( int logicalIndex, bool hide )
{
  header()->setSectionHidden( logicalIndex , hide );
}

bool TreeWidget::isColumnHidden( int logicalIndex ) const
{
  return header()->isSectionHidden( logicalIndex );
}

#define KFOLDERTREEWIDGETBASE_COLUMN_ACTION_ID_BASE 0x10000

bool TreeWidget::fillHeaderContextMenu( KMenu * menu , const QPoint & )
{
  if ( !menu || !mEnableManualColumnHiding )
    return false;

  menu->addTitle( i18n("View Columns") );

  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
    return false; // oops..

  // Dynamically build the menu and check the items for visible columns
  int cc = hitem->columnCount();
  if ( cc < 1 )
    return false;

  for ( int i = 1 ; i < cc; i++ )
  {
    QAction * act = menu->addAction( hitem->text( i ) );
    act->setCheckable( true );
    if ( !header()->isSectionHidden( i ) )
        act->setChecked( true );
    act->setData( QVariant( KFOLDERTREEWIDGETBASE_COLUMN_ACTION_ID_BASE + i ) );
  }

  return true;
}

void TreeWidget::slotToggleColumnActionTriggered( QAction *act )
{
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int id = data.toInt( &ok );
  if ( !ok )
    return;

  if ( id <= KFOLDERTREEWIDGETBASE_COLUMN_ACTION_ID_BASE )
    return;

  id -= KFOLDERTREEWIDGETBASE_COLUMN_ACTION_ID_BASE;

  if ( id > columnCount() )
    return;

  header()->setSectionHidden( id , !act->isChecked() );
}

void TreeWidget::slotHeaderContextMenuRequested( const QPoint &clickPoint )
{
  KMenu menu( this );

  if ( !fillHeaderContextMenu( &menu , clickPoint ) )
    return;

  connect( &menu , SIGNAL( triggered( QAction* ) ) ,
           this , SLOT( slotToggleColumnActionTriggered( QAction* ) ) );

  menu.exec( header()->mapToGlobal( clickPoint ) );
}

int TreeWidget::addColumn( const QString &label , int headerAlignment )
{
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
  {
    // In fact this seems to never happen
    hitem = new QTreeWidgetItem( this );
    hitem->setText( 0 , label );
    hitem->setTextAlignment( 0 , headerAlignment );
    setHeaderItem( hitem ); // will set column count to 1
    return 0;
  }

  int cc = columnCount();

  setColumnCount( cc + 1 );
  hitem->setText( cc , label );
  hitem->setTextAlignment( cc , headerAlignment );
  return cc;
}

bool TreeWidget::setColumnText( int columnIndex , const QString &label )
{
  if ( columnIndex >= columnCount() )
    return false;
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
    return false;
  hitem->setText( columnIndex , label );
  return true;
}

}
