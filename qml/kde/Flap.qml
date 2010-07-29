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
  id : flap_toplevel
  property real leftBound
  property real rightBound
  property real threshold : 0.5
  property bool expander : false

  property alias topBackgroundImage : collapsed_top.source
  property alias midBackgroundImage : collapsed_mid.source
  property alias bottomBackgroundImage : collapsed_bottom.source
  property alias contentArea : _contentArea.data
  property string titleText
  property string titleIcon
  property int imageWidth : 36
  property alias contentWidth : _contentArea.width

  color : "#00000000"

  width : draggedItem.width

  opacity : expander ? ( draggedItem.x - rightBound) / (leftBound - rightBound) : ( draggedItem.x - leftBound ) / (rightBound - leftBound)

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

    width : collapsed_top.width
    height : flap_toplevel.height

    onXChanged : {
      if (x != leftBound && x != rightBound)
        return;

      extensionChanged( ( x - leftBound ) / ( rightBound - leftBound ) )
    }

    // Bug here: http://bugreports.qt.nokia.com/browse/QTBUG-12295
    Behavior on x {
      NumberAnimation {
        easing.type: "OutQuad"
        easing.amplitude: 100
        duration: 300
      }
    }

    Image {
      id : collapsed_top
      anchors.top : parent.top
      anchors.left : parent.left
    }
    Image {
      id : collapsed_mid
      anchors.top : collapsed_top.bottom
      anchors.left : parent.left
      fillMode : Image.TileVertically
      anchors.bottom : collapsed_bottom.top
    }
    Image {
      id : collapsed_bottom
      anchors.bottom : parent.bottom
      anchors.left : parent.left
    }
    Image {
      id: titleImage
      width: imageWidth - 4
      height: (titleIcon == '' ? 0 : width)
      source: titleIcon
      anchors.bottom : parent.bottom
      anchors.right: parent.right
      anchors.margins: 6
    }

    Text {
      id: titleLabel
      height: imageWidth
      z : 100
      anchors.verticalCenter : parent.verticalCenter
      anchors.horizontalCenter : parent.right
      anchors.horizontalCenterOffset : -height + 7
      anchors.verticalCenterOffset : height/ 2
      transformOrigin : Item.Center
      rotation: -90

      text: titleText
      horizontalAlignment: "AlignHCenter"
      verticalAlignment: "AlignVCenter"
    }

    MouseArea {
      id: mrDrag

      property bool active : drag.active

      anchors.top : collapsed_top.top
      anchors.left : collapsed_top.left
      anchors.bottom : collapsed_bottom.bottom
      anchors.right : collapsed_bottom.right

      onReleased: {
        if (draggedItem.x > leftBound + ( ( rightBound - leftBound ) * threshold ) )
        {
          draggedItem.x = rightBound
        } else {
          draggedItem.x = leftBound
        }
      }
      drag.target: parent;
      drag.axis: "XAxis"
      drag.minimumX: leftBound
      drag.maximumX: rightBound
      drag.filterChildren : true

      Item {
        id : _contentArea
        anchors.top : parent.top
        anchors.bottom : parent.bottom
        anchors.right : parent.right
        width : contentWidth
        anchors.leftMargin : 20
        anchors.topMargin : 20
        anchors.bottomMargin : 20
        anchors.rightMargin : 30
      }
    }
  }
}