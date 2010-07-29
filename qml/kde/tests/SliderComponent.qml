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

Item {
  id : _topLevel
  property real leftBound
  property real rightBound
  property real threshold : 0.5
  property bool expander

  Rectangle {
    id : left
    width : 10
    height : _topLevel.height
    x : leftBound - width - 2
    y : _topLevel.y
    color : "red"
  }

  opacity : expander ? ( someRect.x - rightBound) / (leftBound - rightBound) : ( someRect.x - leftBound ) / (rightBound - leftBound)

  signal extensionChanged(real extension)

  function changeExtension(extension)
  {
    console.log("FOO " + someRect.x + " " + leftBound + " " + rightBound)
    if (someRect.x != leftBound && someRect.x != rightBound)
      return;
    console.log("Changed")
    someRect.x = ( extension * (rightBound - leftBound) ) + leftBound
  }

  Rectangle {
    id : someRect
    x : leftBound
    y : _topLevel.y
    color : "lightsteelblue"
    width : 100
    height : 100

    onXChanged : {
      if (x != leftBound && x != rightBound)
        return;

      extensionChanged( ( x - leftBound ) / ( rightBound - leftBound ) )
    }

    Behavior on x {
      NumberAnimation {
        easing.type: "OutQuad"
        easing.amplitude: 100
        duration: 300
      }
    }

    MouseArea {
      id: mrDrag
      //property alias active : drag.active
      anchors.fill: parent
      onReleased: {
        if (someRect.x > leftBound + ( ( rightBound - leftBound ) * threshold ) )
        {
          someRect.x = rightBound
        } else {
          someRect.x = leftBound
        }
      }
      drag.target: parent;
      drag.axis: "XAxis"
      drag.minimumX: leftBound
      drag.maximumX: rightBound
    }
  }

  Rectangle {
    id : right
    width : 10
    height : _topLevel.height
    x : rightBound + someRect.width + 2
    y : _topLevel.y
    color : "red"
  }

  Rectangle {
    width : 1
    height : 10
    x : leftBound + ( ( rightBound - leftBound ) * threshold )
    y : _topLevel.y + _topLevel.height
    color : "red"
  }
}
