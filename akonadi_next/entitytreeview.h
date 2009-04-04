/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_ENTITY_TREE_VIEW
#define AKONADI_ENTITY_TREE_VIEW

#include "akonadi_next_export.h"
#include <QtGui/QTreeView>
class KXmlGuiWindow;
class QDragMoveEvent;

namespace Akonadi
{

class Collection;

/**
 * @short A view to show a collection tree provided by a CollectionModel.
 *
 * When a KXmlGuiWindow is passed to the constructor, the XMLGUI
 * defined context menu @c akonadi_collectionview_contextmenu is
 * used if available.
 *
 * Example:
 *
 * @code
 *
 * class MyWindow : public KXmlGuiWindow
 * {
 *   public:
 *    MyWindow()
 *      : KXmlGuiWindow()
 *    {
 *      Akonadi::CollectionView *view = new Akonadi::CollectionView( this, this );
 *      setCentralWidget( view );
 *
 *      Akonadi::CollectionModel *model = new Akonadi::CollectionModel( this );
 *      view->setModel( model );
 *    }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @author Stephen Kelly <steveire@gmail.com>
 */
class AKONADI_NEXT_EXPORT EntityTreeView : public QTreeView
{
  Q_OBJECT

public:
  /**
   * Creates a new entity tree view.
   *
   * @param parent The parent widget.
   */
  explicit EntityTreeView( QWidget *parent = 0 );

  /**
   * Creates a new entity tree view.
   *
   * @param xmlGuiWindow The KXmlGuiWindow the view is used in.
   *                     This is needed for the XMLGUI based context menu.
   *                     Passing 0 is ok and will disable the builtin context menu.
   * @param parent The parent widget.
   */
  explicit EntityTreeView( KXmlGuiWindow *xmlGuiWindow, QWidget *parent = 0 );

  /**
   * Destroys the entity tree view.
   */
  virtual ~EntityTreeView();

  void showChildCollectionTree( bool show );
  bool childCollectionTreeShown();

  /**
  Reimplemented to hide the child collection tree if that option is set.
  */
  void setRootIndex(const QModelIndex &idx);

  /**
   * Sets the KXmlGuiWindow which the view is used in.
   * This is needed if you want to use the built-in context menu.
   *
   * @param xmlGuiWindow The KXmlGuiWindow the view is used in.
   */
  void setXmlGuiWindow( KXmlGuiWindow *xmlGuiWindow );

  virtual void setModel( QAbstractItemModel * model );

Q_SIGNALS:
  /**
   * This signal is emitted whenever the user has clicked
   * a collection in the view.
   *
   * @param collection The clicked collection.
   */
  void clicked( const Akonadi::Collection &collection );

//     void clicked( const Akonadi::Item &item );

  /**
   * This signal is emitted whenever the current collection
   * in the view has changed.
   *
   * @param collection The new current collection.
   */
  void currentChanged( const Akonadi::Collection &collection );

protected:
  using QTreeView::currentChanged;
  virtual void dragMoveEvent( QDragMoveEvent *event );
  virtual void dragLeaveEvent( QDragLeaveEvent *event );
  virtual void dropEvent( QDropEvent *event );
  virtual void contextMenuEvent( QContextMenuEvent *event );

private:
  //@cond PRIVATE
  class Private;
  Private * const d;

  Q_PRIVATE_SLOT( d, void dragExpand() )
  Q_PRIVATE_SLOT( d, void itemClicked( const QModelIndex& ) )
  Q_PRIVATE_SLOT( d, void itemCurrentChanged( const QModelIndex& ) )
  //@endcond
};

}

#endif
