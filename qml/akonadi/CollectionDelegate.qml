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
  property bool topItem : false
  property int indentation : 0

  signal indexSelected(int row)

  x : indentation
  width : breadcrumbsView.width - indentation

  Item {
    // This is the same as anchors.fill : parent, but ParentAnimation only works
    // if positional layouting is used instead of anchor layouting.
    x : 0
    y : 0
    width : parent.width
    height : parent.height
    id : nestedItem

    MouseArea {
      anchors.fill: parent
      onClicked: {
        if ( fullClickArea )
        {
          if (topItem)
          {
            indexSelected(model.index);
            return;
          }

          if (showChildIndicator)
          {
            nestedItem.state = "before_select_child";
          } else if (!selectedDelegate)
            nestedItem.state = "before_select_breadcrumb";
          indexSelected(model.index);
        }
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
        height : collectionIcon.height
        Text {
          width: wrapper.width - collectionIcon.width - 50
          text : model.display
          //### requires a newer QML version
          //wrapMode: "WrapAnywhere" // Needs the anchors.fill to work properly
        }

        Text {
          x : parent.width - 15
          width : 50
          text : model.unreadCount > 0 ? model.unreadCount : ""
          color : "blue"
        }
      }
    }

    Image {
      width : height
      anchors.right : parent.right
      anchors.rightMargin : 5
      anchors.verticalCenter : parent.verticalCenter
      opacity : ( showChildIndicator && application.childCollectionHasChildren( model.index ) ) ? 1 : 0
      source: "transparentplus.png"
    }

    states : [
      State {
        name : "before_select_child"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 35; y : 0 }
      },
      State {
        name : "before_select_breadcrumb"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 35; y : 0 }
      }
    ]
    transitions : [
      Transition {
        ParentAnimation {
          target : nestedItem
          NumberAnimation {
            properties: "x,y";
            duration : 500
            easing.type: Easing.OutQuad;
          }
        }
      }
    ]
  }
}

