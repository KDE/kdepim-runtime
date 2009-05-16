/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef ITEMFILTERPROXYMODEL_H
#define ITEMFILTERPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include <Akonadi/Item>

class KJob;


/**
  This proxy model should be applied on top of Akonadi::ItemModel for the
  outbox collection.  It will then provide a self-updating model of all items
  which should be sent by the mail dispatcher agent.  I.e., items which:
  * are valid and complete messages and
  * have Status == Queued and
  * have DispatchMode == Immediately or
    have DispatchMode == AfterDueDate and DueDate in the past

  TODO: less generic/confusing name for class?

  TODO: applications will need a model for displaying the items in the outbox
  as well as their Status, DispatchMode, and progress.  Should that be done
  with a simple ItemModel and some kind of fancy Delegate, or should this
  class be extended and put into outboxinterface?
 */
class ItemFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new item proxy filter model.
     *
     * @param parent The parent object.
     */
    explicit ItemFilterProxyModel( QObject *parent = 0 );

    /**
     * Destroys the item proxy filter model.
     */
    virtual ~ItemFilterProxyModel();

    /**
     * Fetches an item from the model, and emits itemFetched() when done.
     * When the dispatcher agent can send a message, it calls this function
     * to get one.
     */
    void fetchAnItem();

  signals:
    void itemFetched( Akonadi::Item &item );

  protected:
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void itemFetchResult( KJob* ) )
    //@endcond

};


#endif
