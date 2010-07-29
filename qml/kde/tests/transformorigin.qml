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

  Grid {
    x : 30
    y : 30
    columns : 3
    spacing : 30
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.TopLeft
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Top
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.TopRight
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Left
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Center
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Right
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.BottomLeft
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Bottom
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : -90
        transformOrigin : Item.BottomRight
      }
    }
  }
}
