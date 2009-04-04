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

#include <kwidgetitemdelegate.h>

#ifndef MICROBLOGDELEGATE_H
#define MICROBLOGDELEGATE_H

class BlogModel;

class MicroblogDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    MicroblogDelegate( QAbstractItemView *itemView, QObject * parent );

    QList<QWidget*> createItemWidgets() const;

    void updateItemWidgets( const QList<QWidget*> widgets,
                            const QStyleOptionViewItem &option,
                            const QPersistentModelIndex &index ) const;

    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;


private slots:
    void slotLinkClicked( const QUrl &url );

private:
    QVariant getData( const BlogModel*, int row, int column ) const;
    QWidget* m_parent;
};

#endif

