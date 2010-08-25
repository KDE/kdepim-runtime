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
  width : 500
  height : 500

  VisualItemModel {
    id : items
    Rectangle {
      id : top1
      width : 100 //ListView.view.width
      height : 30
      color : "red"
      MouseArea {
        anchors.fill : parent
        onClicked : {
          myList.currentIndex = top1.VisualItemModel.index
        }
      }
    }
    Rectangle {
      id : top2
      width : 100 //ListView.view.width
      height : 30
      color : "green"
      MouseArea {
        anchors.fill : parent
        onClicked : {
          myList.currentIndex = top2.VisualItemModel.index
        }
      }
    }
    Rectangle {
      id : top3
      width : 100 //ListView.view.width
      height : 30
      color : "blue"
      MouseArea {
        anchors.fill : parent
        onClicked : {
          myList.currentIndex = top3.VisualItemModel.index
        }
      }
    }
  }

  Component {
    id: highlightBar
    Rectangle {
        x : -10
        width: 120; height: 30
        color: "#66FFFF88"
        y: myList.currentItem.y;
        Behavior on y { NumberAnimation { duration : 100; easing.type : OutQuad } }
    }
  }

  ListView {
    id : myList
    x : 30
    y : 30
    width : 100
    height : 100
    model : items
    focus : true

    delegate : Text { text : model.index; height : 30; width : ListView.view.width }

    highlight : highlightBar
  }
}