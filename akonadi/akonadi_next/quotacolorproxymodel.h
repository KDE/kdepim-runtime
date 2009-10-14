/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_QUOTACOLORPROXYMODEL_H
#define AKONADI_QUOTACOLORPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include "akonadi_next_export.h"

namespace Akonadi {

/**
 * @short A proxy model that colors collection text if they're above a given quota
 * threshold.
 *
 * @code
 *
 *   Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( ... );
 *
 *   Akonadi::QuotaColorProxyModel *proxy = new Akonadi::QuotaColorProxyModel();
 *   proxy->setSourceModel( model );
 *
 *   Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @author Kevin Ottens <ervin@kde.org>
 * @since 4.4
 */
class AKONADI_NEXT_EXPORT QuotaColorProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new quota color proxy model.
     *
     * @param parent The parent object.
     */
    explicit QuotaColorProxyModel( QObject *parent = 0 );

    /**
     * Destroys the statistics tooltip proxy model.
     */
    virtual ~QuotaColorProxyModel();

    void setWarningThreshold( qreal threshold );
    qreal warningThreshold() const;

    void setWarningColor( const QColor &color );
    QColor warningColor() const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    // QAbstractProxyModel does not proxy all methods...
    virtual bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QMimeData* mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
