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
  width : 400
  height : 400

  Rectangle {
    id : dropRect
    x : 130
    y : 30
    width : 90
    height : 150
    color : "green"
  }
  Rectangle {
    x : 30
    y : 30
    width : 90
    height : 150
    id : blueRect
    color : "blue"

    Rectangle {
      x : 10
      y : 30
      width : 70
      height : 90
      color : "yellow"

      Rectangle {
        id : placeholder
        x : 20
        y : 30
        width : 30
        height : 30
//         clip : true
        Rectangle {
          id : targetRect
          width : parent.width
          height : parent.height
          color : "lightsteelblue"
          z : 10000

          MouseArea {
            anchors.fill : parent

            drag.target : targetRect
            drag.axis : Drag.XAxis
            drag.minimumX : 0
            drag.maximumX : 100
          }
        }
      }
    }
  }
}