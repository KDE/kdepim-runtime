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
import org.kde 4.5

Item {
  id: breadcrumbTopLevel
  clip : true

  property alias breadcrumbItemsModel : breadcrumbsView.model
  property alias selectedItemModel : selectedItemView.model
  property alias childItemsModel : childItemsView.model

  property int itemHeight : 68

  signal childCollectionSelected(int row)
  signal breadcrumbCollectionSelected(int row)

  signal collectionSelected

  SystemPalette { id: palette; colorGroup: "Active" }

  Component {
    id : breadcrumbDelegate
    CollectionDelegate {
      fullClickArea : true
      steppedIndent : true
      onIndexSelected : {
        breadcrumbCollectionSelected(row);
      }
    }
  }

  Component {
    id : selectedItemDelegate
    CollectionDelegate {
      itemBackground : palette.dark
      indentOnly : true
    }
  }

  Component {
    id : childItemsDelegate
    CollectionDelegate {
      fullClickArea : true
      showChildIndicator : true
      indentAll : true
      onIndexSelected : {
        childCollectionSelected(row);
      }
    }
  }

  Rectangle {
    id : topButton
    height : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) itemHeight; else 0; }
    opacity : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) 1; else 0; }
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right

    Text { text : "Top" }

    MouseArea {
      anchors.fill : parent
      onClicked : {
        breadcrumbCollectionSelected(-1);
      }
    }
  }

  ListView {
    id : breadcrumbsView
    interactive : false
    property int selectedIndex : -1
    height : { count = ( breadcrumbsView.count > 2 ) ? 2 : breadcrumbsView.count; itemHeight * count }
    anchors.top : topButton.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : breadcrumbDelegate
  }

  ListView {
    id : selectedItemView
    height : itemHeight * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : selectedItemDelegate
  }
  ListView {
    id : childItemsView
    clip : true
    anchors.top : selectedItemView.bottom
    anchors.bottom : breadcrumbTopLevel.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : childItemsDelegate
  }
}
