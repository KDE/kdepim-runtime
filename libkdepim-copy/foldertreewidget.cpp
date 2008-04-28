/******************************************************************************
 *
 * This file is part of libkdepim.
 *
 * Copyright (C) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
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

#include "foldertreewidget.h"

#include <KMenu>
#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QHeaderView>

namespace KPIM {

FolderTreeWidgetBase::FolderTreeWidgetBase( QWidget * parent, const char * name )
: QTreeWidget( parent )
{
  setObjectName( name );
  setColumnCount( 0 );

  header()->setMovable( true );

  setManualColumnHidingEnabled( true );
}

bool FolderTreeWidgetBase::saveLayout( KConfigGroup &group , const char * keyName )
{
  if( !keyName )
    return false;

  group.writeEntry( keyName , QVariant( header()->saveState() ) );

  return true;
}

bool FolderTreeWidgetBase::saveLayout( KConfig * config , const char * groupName , const char * keyName )
{
  if( !config || !groupName || !keyName )
    return false;

  KConfigGroup group( config , groupName );
  return saveLayout( group , keyName );
}

bool FolderTreeWidgetBase::restoreLayout( KConfigGroup &group , const char * keyName )
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
  bool lastSectionWasStretched = header()->stretchLastSection();
  bool sortIndicatorWasShown = header()->isSortIndicatorShown();
  Qt::Alignment originalDefaultAlignment = header()->defaultAlignment();
  int defaultSectionSize = header()->defaultSectionSize();
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

bool FolderTreeWidgetBase::restoreLayout( KConfig * config , const char * groupName , const char * keyName )
{
  if( !config || !groupName || !keyName )
    return false;

  if ( !config->hasGroup( groupName ) )
    return false;

  KConfigGroup group( config , groupName );
  return restoreLayout( group , keyName );
}

void FolderTreeWidgetBase::setManualColumnHidingEnabled( bool enable )
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

void FolderTreeWidgetBase::setColumnHidden( int logicalIndex, bool hide )
{
  header()->setSectionHidden( logicalIndex , hide );
}

bool FolderTreeWidgetBase::isColumnHidden( int logicalIndex ) const
{
  return header()->isSectionHidden( logicalIndex );
}

#define KFOLDERTREEWIDGETBASE_COLUMN_ACTION_ID_BASE 0x10000

bool FolderTreeWidgetBase::fillHeaderContextMenu( KMenu * menu , const QPoint & )
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

void FolderTreeWidgetBase::slotToggleColumnActionTriggered( QAction *act )
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

void FolderTreeWidgetBase::slotHeaderContextMenuRequested( const QPoint &clickPoint )
{
  KMenu menu( this );

  if ( !fillHeaderContextMenu( &menu , clickPoint ) )
    return;

  connect( &menu , SIGNAL( triggered( QAction* ) ) ,
           this , SLOT( slotToggleColumnActionTriggered( QAction* ) ) );

  menu.exec( header()->mapToGlobal( clickPoint ) );
}

int FolderTreeWidgetBase::addColumn( const QString &label )
{
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
  {
    // In fact this seems to never happen
    hitem = new QTreeWidgetItem( this );
    hitem->setText( 0 , label );
    setHeaderItem( hitem ); // will set column count to 1
    return 0;
  }

  int cc = columnCount();

  setColumnCount( cc + 1 );
  hitem->setText( cc , label );
  return cc;
}

bool FolderTreeWidgetBase::setColumnText( int columnIndex , const QString &label )
{
  if ( columnIndex >= columnCount() )
    return false;
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
    return false;
  hitem->setText( columnIndex , label );
  return true;
}






FolderTreeWidget::FolderTreeWidget( QWidget *parent , const char *name )
: FolderTreeWidgetBase( parent , name )
{
  setAlternatingRowColors( true );
  setAcceptDrops( true );
  setAllColumnsShowFocus( true );
  setRootIsDecorated( true );
  setSortingEnabled( true );
  header()->setSortIndicatorShown( true );
  header()->setClickable( true );
  //setSelectionMode( Extended );
}




FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidget *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType
    ) : QTreeWidgetItem( parent ), mProtocol(protocol), mFolderType(folderType)
{
}

FolderTreeWidgetItem::FolderTreeWidgetItem(
        FolderTreeWidgetItem *parent,
        const QString &label,
        Protocol protocol,
        FolderType folderType,
        int unread,
        int total
    ) : QTreeWidgetItem( parent ), mProtocol(protocol), mFolderType(folderType)
{
}

bool FolderTreeWidgetItem::operator < ( const QTreeWidgetItem &other ) const
{
  int sortCol = treeWidget()->sortColumn();
  if ( sortCol < 0 )
     return true; // just "yes" :D

  // WIP

  return text(sortCol) < other.text(sortCol);
}

}
