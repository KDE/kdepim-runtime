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

#ifndef MESSAGEVIEWER_MESSAGEVIEWITEM_H
#define MESSAGEVIEWER_MESSAGEVIEWITEM_H

#include <QtCore/QAbstractItemModel>

#include <akonadi/kcal/incidenceviewer.h>
#include <mobile/lib/declarativeakonadiitem.h>

namespace Akonadi {

class Item;

namespace KCal {

/**
 * @short A wrapper class to make the 'removed' signal available.
 */
class ExtendedIncidenceViewer : public IncidenceViewer
{
  Q_OBJECT

  public:
    ExtendedIncidenceViewer( QWidget *parent = 0 );

  Q_SIGNALS:
    void incidenceRemoved();

  private:
    virtual void itemRemoved();
};

class KCalItemBrowserItem : public DeclarativeAkonadiItem
{
  Q_OBJECT
  Q_PROPERTY( QObject* attachmentModel READ attachmentModel NOTIFY attachmentModelChanged )
  Q_PROPERTY( QDate activeDate WRITE setActiveDate READ activeDate )
  public:
    explicit KCalItemBrowserItem( QDeclarativeItem *parent = 0 );

    virtual Akonadi::Item item() const;
    virtual qint64 itemId() const;
    virtual void setItemId(qint64 id);

    /**
     * Sets the active date for the incidence. The active date is used for
     * incideces that have recurrence.
     */
    QDate activeDate() const;
    void setActiveDate( const QDate &date );

    QObject *attachmentModel() const;

  Q_SIGNALS:
    void attachmentModelChanged();
    void incidenceRemoved();

  private:
    ExtendedIncidenceViewer *m_viewer;
};

}
}

#endif
