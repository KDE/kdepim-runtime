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

  Rectangle {
    x : 30
    y : 30
    color : "lightsteelblue"
    width : 300
    height : 300

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
      drag.target: parent;
      drag.axis: "XAxis"
      drag.minimumX: 30
      drag.maximumX: 130
    }
  }

}
