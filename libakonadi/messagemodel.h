/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef AKONADI_MESSAGEMODEL_H
#define AKONADI_MESSAGEMODEL_H

#include <libakonadi/job.h>
#include <kdepim_export.h>

#include <QtCore/QAbstractTableModel>

namespace Akonadi {

class Job;

/**
  A flat self-updating message model.
*/
class AKONADI_EXPORT MessageModel : public QAbstractTableModel
{
  Q_OBJECT

  public:
    /**
      Column types.
    */
    enum Column {
      Subject, /**< Subject column. */
      Sender, /**< Sender column. */
      Receiver, /**< Receiver column. */
      Date, /**< Date column. */
      Size /**< Size column. */
    };

    /**
      Creates a new message model.

      @param parent The parent object.
    */
    MessageModel( QObject* parent = 0 );

    /**
      Deletes the message model.
    */
    virtual ~MessageModel();

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    /**
      Sets the path to the collection the model should display. If the path has
      changed, the model is reset and a new message listing is requested
      from the storage backend.
      @param path The full collection path.
    */
    void setPath( const QString &path );

    /**
      Returns the message reference to the given model index. If the index
      is invalid, an empty reference is returned.
      @param index The model index.
    */
    DataReference referenceForIndex( const QModelIndex &index ) const;

  private Q_SLOTS:
    /**
      Connected to the message query job which does a complete listing
    */
    void listingDone( Akonadi::Job* job );

    /**
      Connected to message queries that handle newly added messages.
    */
    void fetchingNewDone( Akonadi::Job* job );

    /**
      Connected to message queries that handle changed messages.
    */
    void fetchingUpdatesDone( Akonadi::Job* job );

    /**
      Connected to the monitor job.
    */
    void messagesChanged( const DataReference::List &references );

    /**
      Connected to the monitor job.
    */
    void messagesAdded( const DataReference::List &references );

    /**
      Connected to the monitor job.
    */
    void messagesRemoved( const DataReference::List &references );

  private:
    class Private;
    Private* d;
};

}

#endif
