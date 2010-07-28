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

  property real syncedExtension

  SliderComponent {
    id : one
    y : 30
    x : 50
    height : 100
    leftBound : 30
    rightBound : 130
    threshold : 0.2
    expander : true

    onExtensionChanged : {
      two.changeExtension(extension)
    }
  }
  SliderComponent {
    id : two
    y : 130
    x : 50

    height : 100
    leftBound : 30
    rightBound : 430
    threshold : 0.7
    onExtensionChanged : {
      one.changeExtension(extension)
    }
  }

}
