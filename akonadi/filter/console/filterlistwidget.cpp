/******************************************************************************
 *
 *  File : filterlistwidget.cpp
 *  Created on Fri 07 Aug 2009 03:23:10 by Szymon Tomasz Stefanek
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

#include "filterlistwidget.h"

#include "filter.h"

#include <akonadi/filter/program.h>

#include <QtGui/QAbstractItemDelegate>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QStyleOptionViewItem>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QTextDocument>


class FilterListWidgetDelegate : public QAbstractItemDelegate
{
  public:
    FilterListWidgetDelegate( QObject *parent = 0 );

    virtual void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;

  private:
    void drawFocus( QPainter*, const QStyleOptionViewItem&, const QRect& ) const;

    QTextDocument* document( const QStyleOptionViewItem &option, const QModelIndex &index ) const;
};



FilterListWidgetItem::FilterListWidgetItem( FilterListWidget * parent, Filter * filter )
  : QListWidgetItem( parent ), mFilter( filter )
{
  Q_ASSERT( filter );

  setData( Qt::UserRole, QVariant( reinterpret_cast< qulonglong >( filter ) ) );
  setText( filter->id() );
}

FilterListWidgetItem::~FilterListWidgetItem()
{
  setData( Qt::UserRole, QVariant() );

  delete mFilter;
}



FilterListWidget::FilterListWidget( QWidget * parent )
  : QListWidget( parent )
{
  setItemDelegate( new FilterListWidgetDelegate( this ) );
}

FilterListWidget::~FilterListWidget()
{
}








FilterListWidgetDelegate::FilterListWidgetDelegate( QObject *parent )
 : QAbstractItemDelegate( parent )
{
}

QTextDocument* FilterListWidgetDelegate::document( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return 0;

  QVariant v = index.model()->data( index, Qt::UserRole );
  if( v.isNull() )
    return 0;

  bool ok;
  qulonglong l = v.toULongLong( &ok );
  if( !ok )
    return 0;

  Filter * filter = reinterpret_cast< Filter * >( l ); // BLEAH :D

  QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
  if ( cg == QPalette::Normal && !(option.state & QStyle::State_Active) )
    cg = QPalette::Inactive;

  QColor textColor;
  if ( option.state & QStyle::State_Selected ) {
    textColor = option.palette.color( cg, QPalette::HighlightedText );
  } else {
    textColor = option.palette.color( cg, QPalette::Text );
  }

  return filter->descriptionDocument( textColor.name().toUpper() );
}

void FilterListWidgetDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return;

  QTextDocument *doc = document( option, index );
  if ( !doc )
    return;

  painter->setRenderHint( QPainter::Antialiasing );

  QPen pen = painter->pen();

  QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
  if ( cg == QPalette::Normal && !(option.state & QStyle::State_Active) )
    cg = QPalette::Inactive;

  QStyleOptionViewItemV4 opt(option);
  opt.showDecorationSelected = true;
  QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter );

  painter->save();
  painter->translate( option.rect.topLeft() );
  doc->drawContents( painter );
  painter->restore();

  painter->setPen(pen);

  drawFocus( painter, option, option.rect );
}

QSize FilterListWidgetDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return QSize( 0, 0 );

  QTextDocument *doc = document( option, index );
  if ( !doc )
    return QSize( 0, 0 );

  const QSize size = doc->documentLayout()->documentSize().toSize();

  return size;
}

void FilterListWidgetDelegate::drawFocus( QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect ) const
{
  if ( option.state & QStyle::State_HasFocus ) {
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=( option );
    o.rect = rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color( cg, (option.state & QStyle::State_Selected)
                                                  ? QPalette::Highlight : QPalette::Background );
    QApplication::style()->drawPrimitive( QStyle::PE_FrameFocusRect, &o, painter );
  }
}

