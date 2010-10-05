/*
    Copyright (C) 2010 Anselmo Lacerda Silveira de Melo <anselmolsm@gmail.com>

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

Item {
  id : _topLevel

  property alias content : expandFlap.contentArea

  property int contentWidth : expandFlap.width - 80

  property int expandedPosition : 0
  property int expandedHeight : height - expandedPosition

  property real expandThreshold : 3/4

  signal extensionChanged(real extension)

  function changeExtension(extension)
  {

  }

//  signal expanded(variant obj)
//  signal collapsed(variant obj)

  z: 100
  x : 0
  y : 0

  function collapse() {
    expandFlap.changeExtension(0.0)
  }

  function expand() {
    expandFlap.changeExtension(1.0)
  }

  Flap2 {
    id : expandFlap
    x : -width
    y : expandedPosition
    leftBound : 0
    rightBound : 80 + contentWidth
    height : expandedHeight

    contentWidth : _topLevel.contentWidth

    topBackgroundImage : "flap-expanded-top.png"
    midBackgroundImage : "flap-expanded-mid.png"
    bottomBackgroundImage : "flap-expanded-bottom.png"

    onExtensionChanged : {
      expandFlap.changeExtension(extension)
    }
  }
}
