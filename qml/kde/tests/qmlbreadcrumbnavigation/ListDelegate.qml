/*
 *    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
 *        a KDAB Group company, info@kdab.net,
 *        author Stephen Kelly <stephen@kdab.com>
 *
 *    This library is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU Library General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at your
 *    option) any later version.
 *
 *    This library is distributed in the hope that it will be useful, but WITHOUT
 *    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *    License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to the
 *    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 */

import Qt 4.7

Rectangle {
  property alias text : _text.text;
  property bool clickable : false
  property bool isChild : false
  property bool isSelected : false
  property bool topItem : false

  signal indexSelected(int row)

  color : "#00000000"
  height: itemHeight;
  width : ListView.view.width

  Item {
    // This is the same as anchors.fill : parent, but ParentAnimation only works
    // if positional layouting is used instead of anchor layouting.
    x : 0
    y : 0
    width : parent.width
    height : parent.height
    id : nestedItem

    Text {
      id : _text
      anchors.verticalCenter : parent.verticalCenter;
      anchors.horizontalCenter : parent.horizontalCenter;
      text : model.display
    }

    Image {
      width : height
      anchors.right : parent.right
      anchors.rightMargin : 5
      anchors.verticalCenter : parent.verticalCenter
      opacity : ( isChild && application.childCollectionHasChildren( model.index ) ) ? 1 : 0
      source: "transparentplus.png"
    }

    MouseArea {
      anchors.fill : parent
      onClicked: {
        if ( clickable )
        {
          if (topItem)
          {
            indexSelected(model.index);
            return;
          }

          if (isChild)
          {
            nestedItem.state = "before_select_child";
          } else if (!isSelected)
            nestedItem.state = "before_select_breadcrumb";
          indexSelected(model.index);
        }
      }
    }

    states : [
      State {
        name : "before_select_child"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 0; y : 0 }
      },
      State {
        name : "before_select_breadcrumb"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 0; y : 0 }
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