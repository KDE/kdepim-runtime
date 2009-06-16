/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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


#ifndef AKONADI_ABSTRACTPROXYMODEL_H
#define AKONADI_ABSTRACTPROXYMODEL_H

// WARNING: Changes to this class will probably also have to be made to AbstractItemModel

#include <QtGui/QAbstractProxyModel>

#include "akonadi_next_export.h"

class AbstractProxyModelPrivate;

/**
 * This class should be subclassed to create concrete proxy models which are aware of
 * move operations reported by an AbstractEntityModel.
 */
class AKONADI_NEXT_EXPORT AbstractProxyModel : public QAbstractProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new abstract proxy model.
     *
     * @param parent The parent object.
     */
    AbstractProxyModel( QObject *parent = 0 );

    /**
     * Destroys the abstract proxy model.
     */
    virtual ~AbstractProxyModel();

    /**
     * Sets the header @p set that shall be used by the proxy.
     *
     * \s EntityTreeModel::HeaderGroup
     */
    void setHeaderSet( int set );

    /**
     * Returns the header set used by the proxy.
     */
    int headerSet() const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    // QAbstractProxyModel does not proxy all methods...
    virtual bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QMimeData* mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes() const;

  protected:
    void beginMoveRows( const QModelIndex &srcParent, int start, int end,
                        const QModelIndex &destinationParent, int destinationRow);
    void endMoveRows();

    void beginChangeChildOrder( const QModelIndex &index );
    void endChangeChildOrder();

    void beginResetModel();
    void endResetModel();

  signals:
#if !defined(Q_MOC_RUN) && !defined(qdoc)
  private: // can only be emitted by AbstractItemModel
#endif

    void rowsAboutToBeMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row );
    void rowsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row );

    void columnsAboutToBeMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column );
    void columnsMoved( const QModelIndex &parent, int start, int end, const QModelIndex &destination, int column );

    void childOrderAboutToBeChanged( const QModelIndex &index );
    void childOrderChanged( const QModelIndex &index );

  private:
    Q_DECLARE_PRIVATE( AbstractProxyModel )

    AbstractProxyModelPrivate *d_ptr;
};

#endif
