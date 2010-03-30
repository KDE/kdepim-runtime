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

  property alias breadcrumbItemsModel : breadcrumbsView.model
  property alias selectedItemModel : selectedItemView.model
  property alias childItemsModel : childItemsView.model

  property alias breadcrumbIndex : breadcrumbsView.selectedIndex
  property alias childItemsIndex : childItemsView.selectedIndex

  signal childCollectionSelected(int row)
  signal breadcrumbCollectionSelected(int row)

  signal collectionSelected

  SystemPalette { id: palette; colorGroup: "Active" }

  CollectionDelegate {
    id : breadcrumbDelegate

//     onIndexSelected : {
//       console.log( "second" );
      //breadcrumbCollectionSelected(row);
//     }
  }
/*
  Connections {
    target : breadcrumbDelegate
    onIndexSelected : {
      console.log( "second" );
      //breadcrumbCollectionSelected(row);
    }
  }
*/
  CollectionDelegate3 {
    id : selectedItemDelegate
  }

  CollectionDelegate2 {
    id : childItemsDelegate
//     onIndexSelected : {
//         console.log( "second" + row );
//        childCollectionSelected(row);
//     }
  }

  ListView {
    id : breadcrumbsView
    property int selectedIndex : -1
    height : 68 * breadcrumbsView.count
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : breadcrumbDelegate
  }

  ListView {
    id : selectedItemView
    property int selectedIndex : -1
    height : 68 * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
    delegate : selectedItemDelegate
  }
  ListView {
    id : childItemsView
    property int selectedIndex : -1
    anchors.top : selectedItemView.bottom
    height : 68 * 4
    delegate : childItemsDelegate
  }
}
