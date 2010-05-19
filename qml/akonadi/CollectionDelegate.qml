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
import org.kde.akonadi 4.5

Item {
  id: wrapper
//   clip: true

  property bool fullClickArea : false
  property bool showChildIndicator : false
  property bool selectedDelegate : false

  signal indexSelected(int row)

  width : breadcrumbsView.width

  Image {
    id : fuzz
    source : "fuzz.png"
    anchors.horizontalCenter : parent.horizontalCenter
    y : parent.height / 2
    opacity : (selectedDelegate && hasChildren) ? 1 : 0
  }
  Image {
    anchors.right : parent.right
    anchors.rightMargin : 5
    anchors.verticalCenter : parent.verticalCenter
    opacity : (selectedDelegate && hasChildren) ? 1 : 0
    source: "currentindicator.png"
  }
  Image {
    id : lastItemImage
    opacity : (selectedDelegate && !hasChildren) ? 1 : 0;
    source : "selected_bottom.png"
    anchors.horizontalCenter : parent.horizontalCenter
    y : parent.height / 2
  }

  MouseArea {
    anchors.fill: parent
    onClicked: {
      if ( fullClickArea )
        indexSelected(model.index);
    }
  }
  Row {
    id: topLayout
    x: 10; y: 10;
    height: collectionIcon.height;
    width: parent.width
    spacing: 10

    Image {
        id: collectionIcon
        pixmap: KDE.iconToPixmap( model.decoration, height );
        width: 48; height: 48
    }

    Column {
      height: collectionIcon.height
      width: wrapper.width - collectionIcon.width - 20
      spacing: 5
      Text {
        anchors.fill: parent
        text : model.display
        //### requires a newer QML version
        //wrapMode: "WrapAnywhere" // Needs the anchors.fill to work properly
        font.bold: true
      }
    }
    Text {
      anchors.bottom : parent.bottom
      anchors.right : parent.right
      anchors.rightMargin : 15 + parent.spacing
      text : model.unreadCount > 0 ? model.unreadCount : ""
      color : "blue"
    }
  }

  Image {
    width : height
    anchors.right : parent.right
    anchors.rightMargin : 5
    anchors.top : parent.top
    anchors.topMargin : 5
    opacity : ( showChildIndicator && application.childCollectionHasChildren( model.index ) ) ? 1 : 0
    source: "transparentplus.png"
  }
}

