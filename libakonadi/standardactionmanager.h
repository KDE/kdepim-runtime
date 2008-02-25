/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_STANDARDACTIONMANAGER_H
#define AKONADI_STANDARDACTIONMANAGER_H

#include "libakonadi_export.h"

#include <QtCore/QObject>

class KAction;
class KActionCollection;
class QItemSelectionModel;
class QWidget;

namespace Akonadi {

/**
  Manages generic Akonadi actions common for all types. This covers
  creating of the actions with appropriate labels, icons, shortcuts
  etc., updating the action state depending on the current selection
  as well as default implementations for the actual operations.

  If the default implementation is not appropriate for your application
  you can still use the state tracking by disconnecting the triggered()
  signal and re-connecting it to your implementation. The actual KAction
  objects can be retrieved by calling createAction() or action() for that.

  The following actions are provided (KAction name in parenthesis):
  - Creation of a new collection (@c akonadi_collection_create)
  - Copying of selected collections (@c akonadi_collection_copy)
  - Deletion of selected collections (@c akonadi_collection_delete)
  - Synchronization of selected collections (@c akonadi_collection_sync)
  - Showing the collection properties dialog for the current collection (@c akonadi_collection_properties)

  The following example shows how to use standard actions in your application:
@verbatim
  Akonadi::StandardActionManager *actMgr = new Akonadi::StandardActionManager( actionCollection(), this );
  actMgr->setCollectionSelectionModel( collectionView->collectionSelectionModel() );
  actMgr->createAllActions();
@endverbatim
  Additionally you have to add the actions to the KXMLGUI file of your application,
  using the names listed above.

  @todo collection deleting, copy and sync do not support multi-selection yet
*/
class AKONADI_EXPORT StandardActionManager : public QObject
{
  Q_OBJECT
  public:
    /** List of supported actions. */
    enum Type {
      CreateCollection,
      CopyCollection,
      DeleteCollection,
      SynchronizeCollection,
      CollectionProperties,
      LastType
    };

    /**
      Create a new standard action manager.
      @param actionCollection The action collection to operate on.
      @param parent The parent widget.
    */
    StandardActionManager( KActionCollection *actionCollection, QWidget *parent = 0 );

    /**
      Destructor.
    */
    ~StandardActionManager();

    /**
      Sets the collection selection model based on which the collection
      related actions should operate. If none is set, all collection actions
      will be disabled.
    */
    void setCollectionSelectionModel( QItemSelectionModel *selectionModel );

    /**
      Creates the action of the given type and adds it to the action collection
      specified in the constructor if it does not exist yet. The action is
      connected to its default implementation provided by this class.
    */
    KAction* createAction( Type type );

    /**
      Convenience method to create all standard actions.
      @see createAction()
    */
    void createAllActions();

    /**
      Returns the action of the given type, 0 if it has not been created (yet).
    */
    KAction* action( Type type ) const;

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void collectionSelectionChanged(QItemSelection, QItemSelection) )

    Q_PRIVATE_SLOT( d, void slotCreateCollection() )
    Q_PRIVATE_SLOT( d, void slotCopyCollection() )
    Q_PRIVATE_SLOT( d, void slotDeleteCollection() )
    Q_PRIVATE_SLOT( d, void slotSynchronizeCollection() )
    Q_PRIVATE_SLOT( d, void slotCollectionProperties() )

    Q_PRIVATE_SLOT( d, void collectionCreationResult(KJob*) )
    Q_PRIVATE_SLOT( d, void collectionDeletionResult(KJob*) )
};

}


#endif
