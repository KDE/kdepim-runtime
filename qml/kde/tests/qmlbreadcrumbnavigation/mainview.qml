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
  gradient: Gradient {
    GradientStop { position: 0.0; color: "lightgrey" }
    GradientStop { position: 0.5; color: "grey" }
  }
  anchors.fill : parent

  BreadcrumbNavigationView {

    SystemPalette { id: palette; colorGroup: "Active" }

    anchors.fill : parent;

    topDelegate : Rectangle
    {
      color : "#00000000"
      height: itemHeight;
      width : ListView.view.width
      Text {
        anchors.verticalCenter : parent.verticalCenter;
        anchors.horizontalCenter : parent.horizontalCenter;
        text : model.display
      }
      MouseArea {
        anchors.fill : parent
        onClicked : { breadcrumbTopLevel._transitionSelect = -1; breadcrumbTopLevel.state = "before_select_breadcrumb"; }
      }
    }

    breadcrumbDelegate : Rectangle
    {
      color : "#00000000"
      height: itemHeight;
      width : ListView.view.width
      Text {
        anchors.verticalCenter : parent.verticalCenter;
        anchors.horizontalCenter : parent.horizontalCenter;
        text : model.display
      }
      MouseArea {
        anchors.fill : parent
        onClicked : { breadcrumbTopLevel._transitionSelect = model.index; breadcrumbTopLevel.state = "before_select_breadcrumb"; }
      }
    }

    selectedItemDelegate : Rectangle
    {
      color : "#00000000"
      height: itemHeight;
      width : ListView.view.width
      Text {
        anchors.verticalCenter : parent.verticalCenter;
        anchors.horizontalCenter : parent.horizontalCenter;
        text : model.display
      }

      Rectangle {
        id: topLine
        x: 1; y: 2; width: parent.width - 2; height: 2
        color: "grey"
      }
      Rectangle {
        id: bottomLine
        x: 1; y: parent.height - 4; width: parent.width - 2; height: 2
        color: "darkgrey"
      }
    }

    childItemsDelegate : Rectangle
    {
      color : "#00000000"
      height: itemHeight;
      width : ListView.view.width

      Text {
        anchors.verticalCenter : parent.verticalCenter;
        anchors.horizontalCenter : parent.horizontalCenter;
        text : model.display
      }
      MouseArea {
        anchors.fill : parent
        onClicked : { breadcrumbTopLevel._transitionSelect = model.index; breadcrumbTopLevel.state = "before_select_child"; }
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
}
