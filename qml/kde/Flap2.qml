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

Rectangle {
  id : flap_toplevel
  property real leftBound
  property real rightBound

  property alias topBackgroundImage : topImage.source
  property alias midBackgroundImage : midImage.source
  property alias bottomBackgroundImage : bottomImage.source

  property alias contentArea : _contentArea.data
  property alias contentWidth : _contentArea.width

  color : "#00000000"

  width : draggedItem.width

  opacity : ( draggedItem.x - leftBound ) / (rightBound - leftBound)

  signal extensionChanged(real extension)

  function changeExtension(extension)
  {
    if (draggedItem.x != leftBound && draggedItem.x != rightBound)
      return;

    draggedItem.x = ( extension * (rightBound - leftBound) ) + leftBound
  }

  Item {
    id : draggedItem
    x : leftBound
    y : 0

    width : topImage.width
    height : flap_toplevel.height

    onXChanged : {
      if (x != leftBound && x != rightBound)
        return;

      extensionChanged( ( x - leftBound ) / ( rightBound - leftBound ) )
    }

    Behavior on x {
      PropertyAnimation {
        duration: 300
      }
    }

    Image {
      id : topImage
      anchors {
        top : parent.top
        left : parent.left
      }
    }
    Image {
      id : midImage
      anchors {
        top : topImage.bottom
        left : parent.left
        bottom : bottomImage.top
      }
      fillMode : Image.TileVertically
    }
    Image {
      id : bottomImage
      anchors {
        bottom : parent.bottom
        left : parent.left
      }
    }

    Item {
      id : _contentArea
      width : contentWidth
      anchors {
        top : parent.top
        bottom : parent.bottom
        right : parent.right

        leftMargin : 20
        topMargin : 20
        bottomMargin : 20
        rightMargin : 30
      }
    }
  }
}
