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
#include <QVector>
#include <QEvent>

namespace KPIM {

#define KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY "TreeWidgetLayout"

TreeWidget::TreeWidget( QWidget * parent, const char * name )
: QTreeWidget( parent )
{
  setObjectName( name );
  setColumnCount( 0 );

  header()->setMovable( true );

  setManualColumnHidingEnabled( true );

  // At the moment of writing (25.11.2008) there is a bug in Qt
  // which causes an assertion failure in animated views when
  // the animated item is deleted. The bug notification has been
  // sent to qt-bugs. For the moment we keep animations explicitly disabled.
  // FIXME: Re-enable animations once the qt bug is fixed.
  setAnimated( false );
}

void TreeWidget::changeEvent( QEvent *e )
{
  QTreeWidget::changeEvent( e );

  if ( e->type() == QEvent::StyleChange )
  {
    // At the moment of writing (25.11.2008) there is a bug in Qt
    // which causes an assertion failure in animated views when
    // the animated item is deleted. The bug notification has been
    // sent to qt-bugs. For the moment we keep animations explicitly disabled.
    // FIXME: Re-enable animations once the qt bug is fixed.
    setAnimated( false );
  }
}

bool TreeWidget::saveLayout( KConfigGroup &group, const QString &keyName ) const
{
  group.writeEntry(
      keyName.isEmpty() ? QString( KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY ) : keyName,
      QVariant( header()->saveState().toHex() )
    );

  return true;
}

bool TreeWidget::saveLayout( KConfig * config, const QString &groupName, const QString &keyName ) const
{
  if( !config || groupName.isEmpty() )
    return false;

  KConfigGroup group( config, groupName );
  return saveLayout( group, keyName );
}

bool TreeWidget::restoreLayout( KConfigGroup &group, const QString &keyName )
{
  // Restore the view column order, width and visibility
  QByteArray state = group.readEntry(
                             keyName.isEmpty() ? QString( KPIM_TREEWIDGET_DEFAULT_CONFIG_KEY ) : keyName,
                             QVariant( QVariant::ByteArray )
                         ).toByteArray();
  state = QByteArray::fromHex( state );

  if( state.isEmpty() )
    return false;

  int cc = header()->count();

  // Qt writes also some options that can be set only
  // programmatically. This can drive you crazy if you
  // first setup the view and then restore the layout:
  // it will overwrite your previous settings...

  bool sectionsWereClickable = header()->isClickable();
  bool sectionsWereMovable = header()->isMovable();
  bool sortIndicatorWasShown = header()->isSortIndicatorShown();
  Qt::Alignment originalDefaultAlignment = header()->defaultAlignment();
  //int defaultSectionSize = header()->defaultSectionSize();
  int minimumSectionSize = header()->minimumSectionSize();
  bool cascadingSectionResizes = header()->cascadingSectionResizes();
  QVector<QHeaderView::ResizeMode> resizeModes( cc );
  for ( int i = 0 ; i < cc ; ++i ) {
    resizeModes[ i ] = header()->resizeMode( i );
  }

  // Qt (4.4) will perform a layout based on the stored column sizes
  // when restoreState() is called. However, there is an issue
  // related to the last section: the restoreState() call will
  // preserve the _current_ (pre-restoreState()-call) width
  // of the last section if that is actually greater than the stored
  // value. This is very likely to throw the last section out of
  // the view and cause a horizontal scroll bar to appear even
  // if it wasn't present when saveState() was called.
  // This seems to be a Qt buggie and we workaround it by
  // setting all the sections sizes to something very small
  // just before calling restoreState().

  // we also need to save the current sections sizes in order to remain
  // consistent if restoreState() fails for some reason.
  QVector<int> savedSizes( cc );
  for ( int i = 0 ; i < cc ; i++ )
  {
     savedSizes[ i ] = header()->sectionSize( i );
    header()->resizeSection( i , 10 );
  }

  if ( !header()->restoreState( state ) )
  {
    // failed: be consistent and restore the section sizes before returning
    for ( int c = 0 ; c < cc ; c++ )
      header()->resizeSection( c , savedSizes[ c ] );
    return false;
  }

  header()->setClickable( sectionsWereClickable );
  header()->setMovable( sectionsWereMovable );
  header()->setSortIndicatorShown( sortIndicatorWasShown );
  header()->setDefaultAlignment( originalDefaultAlignment );
  header()->setMinimumSectionSize( minimumSectionSize );
  header()->setCascadingSectionResizes( cascadingSectionResizes );
  for ( int i = 0 ; i < cc ; ++i ) {
    header()->setResizeMode( i, resizeModes[ i ] );
  }

  // FIXME: This would cause the sections to be resized and thus
  //        can't be reliably reset after the configuration
  //        has been read. Can do nothing about that except warning
  //        the user in the docs.
  //header()->setDefaultSectionSize( defaultSectionSize );

  return true;
}

bool TreeWidget::restoreLayout( KConfig * config, const QString &groupName, const QString &keyName )
{
  if( !config || groupName.isEmpty() )
    return false;

  if ( !config->hasGroup( groupName ) )
    return false;

  KConfigGroup group( config, groupName );
  return restoreLayout( group, keyName );
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
  if ( header()->isSectionHidden( logicalIndex ) == hide )
    return;
  header()->setSectionHidden( logicalIndex, hide );

  emit columnVisibilityChanged( logicalIndex );
}

bool TreeWidget::isColumnHidden( int logicalIndex ) const
{
  return header()->isSectionHidden( logicalIndex );
}

bool TreeWidget::fillHeaderContextMenu( KMenu * menu, const QPoint & )
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
    act->setData( QVariant( i ) );

    connect(
        act, SIGNAL( triggered( bool ) ),
        this, SLOT( slotToggleColumnActionTriggered( bool ) ) );
  }

  return true;
}

void TreeWidget::toggleColumn( int logicalIndex )
{
  setColumnHidden( logicalIndex, !header()->isSectionHidden( logicalIndex ) );
}


void TreeWidget::slotToggleColumnActionTriggered( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int id = data.toInt( &ok );
  if ( !ok )
    return;

  if ( id > columnCount() )
    return;

  setColumnHidden( id, !act->isChecked() );
}

void TreeWidget::slotHeaderContextMenuRequested( const QPoint &clickPoint )
{
  KMenu menu( this );

  if ( !fillHeaderContextMenu( &menu, clickPoint ) )
    return;

  menu.exec( header()->mapToGlobal( clickPoint ) );
}

int TreeWidget::addColumn( const QString &label, int headerAlignment )
{
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
  {
    // In fact this seems to never happen
    hitem = new QTreeWidgetItem( this );
    hitem->setText( 0, label );
    hitem->setTextAlignment( 0, headerAlignment );
    setHeaderItem( hitem ); // will set column count to 1
    return 0;
  }

  int cc = columnCount();

  setColumnCount( cc + 1 );
  hitem->setText( cc, label );
  hitem->setTextAlignment( cc, headerAlignment );
  return cc;
}

bool TreeWidget::setColumnText( int columnIndex, const QString &label )
{
  if ( columnIndex >= columnCount() )
    return false;
  QTreeWidgetItem * hitem = headerItem();
  if ( !hitem )
    return false;
  hitem->setText( columnIndex, label );
  return true;
}

QTreeWidgetItem * TreeWidget::firstItem() const
{
  if ( topLevelItemCount() < 1 )
    return 0;
  return topLevelItem( 0 );
}

QTreeWidgetItem * TreeWidget::lastItem() const
{
  QTreeWidgetItem *it = 0;
  int cc = topLevelItemCount();
  if ( cc < 1 )
    return 0;
  it = topLevelItem( cc - 1 );
  if ( !it )
    return 0; // sanity check
  cc = it->childCount();
  while ( cc > 0 )
  {
    it = it->child( cc - 1 );
    if ( !it )
      return 0; // aaaaaargh: something is really wrong :D
    cc = it->childCount();
  }
  return it;
}

}
