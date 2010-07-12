/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef DECLARATIVEWIDGETWRAPPER_H
#define DECLARATIVEWIDGETWRAPPER_H

#include <qdeclarativeitem.h>

class QGraphicsProxyWidget;

namespace Qt {

class DeclarativeWidgetWrapperBase : public QDeclarativeItem
{
  Q_OBJECT
  Q_PROPERTY( QString styleSheet READ styleSheet WRITE setStyleSheet )

  public:
    DeclarativeWidgetWrapperBase( QWidget *widget, QDeclarativeItem *parent );

    QString styleSheet() const { return widget()->styleSheet(); }
    void setStyleSheet( const QString &styleSheet ) { widget()->setStyleSheet( styleSheet ); }

  protected:
    void geometryChanged( const QRectF &newGeometry, const QRectF &oldGeometry );

    virtual QWidget* widget() const = 0;

  private:
    QGraphicsProxyWidget *m_proxy;
};

template <typename WidgetT>
class DeclarativeWidgetWrapper : public DeclarativeWidgetWrapperBase
{
  public:
    explicit DeclarativeWidgetWrapper( QDeclarativeItem* parent ) :
      DeclarativeWidgetWrapperBase( m_widget = new WidgetT, parent )
    {
    }

  protected:
    WidgetT* widget() const { return m_widget; }
    WidgetT* m_widget;
};

}


#endif