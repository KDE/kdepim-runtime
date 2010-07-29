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
  width : 800
  height : 480

  Flickable {
    x : 400
    y : 0
    width : 100
    height : 350
    clip : true
    flickableDirection: Flickable.VerticalFlick
    contentHeight: myColumn.height;

    Column {
      id : myColumn
      Text { height : 20; width : 100; text : "a" }
      Text { height : 20; width : 100; text : "b" }
      Text { height : 20; width : 100; text : "c" }
      Text { height : 20; width : 100; text : "d" }
      Text { height : 20; width : 100; text : "e" }
      Text { height : 20; width : 100; text : "f" }
      Text { height : 20; width : 100; text : "g" }
      Text { height : 20; width : 100; text : "h" }
      Text { height : 20; width : 100; text : "i" }
      Text { height : 20; width : 100; text : "j" }
      Text { height : 20; width : 100; text : "k" }
      Text { height : 20; width : 100; text : "l" }
      Text { height : 20; width : 100; text : "m" }
      Text { height : 20; width : 100; text : "n" }
      Text { height : 20; width : 100; text : "o" }
      Text { height : 20; width : 100; text : "p" }
      Text { height : 20; width : 100; text : "q" }
      Text { height : 20; width : 100; text : "r" }
      Text { height : 20; width : 100; text : "s" }
      Text { height : 20; width : 100; text : "t" }
      Text { height : 20; width : 100; text : "u" }
      Text { height : 20; width : 100; text : "v" }
      Text { height : 20; width : 100; text : "w" }
      Text { height : 20; width : 100; text : "x" }
      Text { height : 20; width : 100; text : "y" }
      Text { height : 20; width : 100; text : "z" }
    }
  }

  Rectangle {
    x : 30
    y : 30
    color : "lightsteelblue"
    width : 300
    height : 300

    MouseArea {
      id: mrDrag
      anchors.fill: parent

      onClicked : {
        mouse.accepted = false;
        return;
      }

      onReleased: {
        if (!drag.active)
        {
          mouse.accepted = false;
          return;
        }
      }

      Rectangle {
        color : "yellow"
        x : 50
        y : 50
          z : 1000
        width : 100
        height : 100


        MouseArea {
          anchors.fill : parent
          onClicked : {
            console.log("Clicked");
          }
          onPressed : {
            console.log("P")
            mouse.accepted = true
          }
          onPressAndHold : {
            console.log("P AND H")
            mouse.accepted = false
          }
        }
      }

      drag.target: parent;
      drag.axis: "XAxis"
      drag.filterChildren : true
      drag.minimumX: 30
      drag.maximumX: 430
    }
  }

}
