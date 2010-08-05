/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

import Qt 4.7

Rectangle {
  color : "#f6f6f5"
  anchors.fill : parent

  Image {
    id : backgroundImage
    width : parent.width
    fillMode : Image.TileHorizontally
    source : "backgroundtile.png"
  }

  BreadcrumbNavigationView {
    id : breadcrumbNavView
    SystemPalette { id: palette; colorGroup: "Active" }

    anchors.top : parent.top;
    anchors.bottom : parent.bottom;
    anchors.left : parent.left;
    width : 200

// anchors.rightMargin : 20

    topDelegate : ListDelegate
    {
      clickable : true
      topItem : true
      onIndexSelected : {
        breadcrumbTopLevel._transitionSelect = -1;
        breadcrumbTopLevel.state = "before_select_home";
      }
    }

    breadcrumbDelegate : ListDelegate
    {
      clickable : true
      selectionModel : _breadcrumbCheckModel
      onIndexSelected : {
        breadcrumbTopLevel._transitionSelect = row;
        breadcrumbTopLevel.state = "before_select_breadcrumb";
      }
    }

    selectedItemDelegate : ListDelegate
    {
      isSelected : true
      selectionModel : _selectedItemCheckModel
    }

    childItemsDelegate : ListDelegate
    {
      clickable : true
      isChild : true
      selectionModel : _childCheckModel
      onIndexSelected : {
        breadcrumbTopLevel._transitionSelect = row;
        breadcrumbTopLevel.state = "before_select_child";
      }
    }

    breadcrumbItemsModel : _breadcrumbItemsModel
    selectedItemModel : _selectedItemModel
    childItemsModel : _childItemsModel

    onChildCollectionSelected :
    {
      application.setSelectedChildCollectionRow( row );
    }

    onBreadcrumbCollectionSelected :
    {
      application.setSelectedBreadcrumbCollectionRow( row );
    }
  }

  ListView {
    anchors.top : breadcrumbNavView.top;
    anchors.bottom : breadcrumbNavView.bottom;
    anchors.left : breadcrumbNavView.right;
    width : 100
    model : _selectedItemsModel
    delegate: ListDelegate
    {
      height : 67
      selectionModel : _selectedItemsSelectionModel
    }
  }
}
