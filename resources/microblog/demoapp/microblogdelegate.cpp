/*
    Copyright (c) 2009 Omat Holding B.V. <info@omat.nl>

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

#include <QtGui>

#include <KDebug>
#include <KLocale>
#include <QWebView>

#include "microblogdelegate.h"
#include "blogmodel.h"

MicroblogDelegate::MicroblogDelegate( QAbstractItemView *itemView, QObject * parent )
        : KWidgetItemDelegate( itemView, parent ), m_parent( itemView )
{
}

QList<QWidget*> MicroblogDelegate::createItemWidgets() const
{
    QList<QWidget*> list;

    QWebView * infoLabel = new QWebView();
    infoLabel->setBackgroundRole( QPalette::NoRole );
    list << infoLabel;
    return list;
}

void MicroblogDelegate::updateItemWidgets( const QList<QWidget*> widgets,
        const QStyleOptionViewItem &option,
        const QPersistentModelIndex &index ) const
{
    Q_UNUSED( option );
    if ( !index.isValid() ) {
        return;
    }
    const BlogModel* model = static_cast<const BlogModel*>( index.model() );

    QWebView *edit = static_cast<QWebView*>( widgets[0] );
    edit->move( 5, 5 );
    edit->resize( 400,200 );

    QString text;
    text.append( "<table><tr><td><img src=\"" + model->data( model->index( index.row(),3 ) ,Qt::DisplayRole ).toString() + "\"></td>" ) ;
    text.append( "<td>" + model->data( model->index( index.row(),0 ) ,Qt::DisplayRole ).toString() );
    text.append( "<Br>" + model->data( model->index( index.row(),2 ) ,Qt::DisplayRole ).toString() ) ;
    text.append( "</td></tr></table>" );
    text.append( "<Br>" + model->data( model->index( index.row(),1 ) ,Qt::DisplayRole ).toString() ) ;
    //kDebug() << text;
    edit->setHtml( text );
}

void MicroblogDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    painter->save();

    if ( option.state & QStyle::State_Selected ) {
        painter->fillRect( option.rect, option.palette.highlight() );
    } else {
        painter->fillRect( option.rect, ( index.row() % 2 == 0 ? option.palette.base() : option.palette.alternateBase() ) );
        painter->setPen( QPen( option.palette.window().color() ) );
        painter->drawRect( option.rect );
    }

    if ( option.state & QStyle::State_Selected ) {
        painter->setPen( QPen( option.palette.highlightedText().color() ) );
    } else {
        painter->setPen( QPen( option.palette.text().color() ) );
    }

    painter->restore();
}


QSize MicroblogDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    return QSize( 410, 210 );
}
#include "microblogdelegate.moc"
