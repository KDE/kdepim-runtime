/*
    This file is part of KContactManager.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtGui/QWidget>

namespace Akonadi {
class CollectionFilterProxyModel;
class CollectionModel;
class CollectionView;
class DataReference;
class ItemView;
}

class KABCModel;
class KABCItemBrowser;
class KXMLGUIClient;

class MainWidget : public QWidget
{
  Q_OBJECT

  public:
    MainWidget( KXMLGUIClient *guiClient, QWidget *parent = 0 );
    ~MainWidget();

  private Q_SLOTS:
    void newContact();
    void newGroup();

    void editItem( const Akonadi::DataReference &item );

  private:
    void setupGui();
    void setupActions();

    void editContact( const Akonadi::DataReference &contact );
    void editGroup( const Akonadi::DataReference &group );

    Akonadi::CollectionModel *mCollectionModel;
    Akonadi::CollectionFilterProxyModel *mCollectionFilterModel;
    Akonadi::ItemView *mContactView;
    KABCItemBrowser *mContactDetails;

    Akonadi::CollectionView *mCollectionView;
    KABCModel *mContactModel;

    KXMLGUIClient *mGuiClient;
};

#endif
