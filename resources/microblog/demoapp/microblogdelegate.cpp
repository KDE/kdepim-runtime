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

MicroblogDelegate::MicroblogDelegate( QObject *parent, QListView *listView )
        : KWidgetItemDelegate( listView, parent ), m_parent( listView )
{
}

QList<QWidget*> MicroblogDelegate::createItemWidgets() const
{
    QList<QWidget*> widgetList;

    QWebView* edit = new QWebView( m_parent );
    edit->page()->view()->setMaximumWidth( 200 );
    edit->page()->view()->setMaximumHeight( 200 );
    widgetList << edit;
    return widgetList;
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
    QString text;
    text.append( "<table><tr><td><img src=\"" + model->data( model->index( index.row(),3 ) ,Qt::DisplayRole ).toString() + "\"></td>" ) ;
    text.append( "<td>" + model->data( model->index( index.row(),0 ) ,Qt::DisplayRole ).toString() );
    text.append( "<Br>" + model->data( model->index( index.row(),2 ) ,Qt::DisplayRole ).toString() ) ;
    text.append( "</td></tr></table>" );
    text.append( "<Br>" + model->data( model->index( index.row(),1 ) ,Qt::DisplayRole ).toString() ) ;
    kDebug() << text;
    edit->setHtml( text );
}

void MicroblogDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    KWidgetItemDelegate::paintWidgets( painter, option, index );
}

QSize MicroblogDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );
    return QSize( 200, 200 );
}
#include "microblogdelegate.moc"
