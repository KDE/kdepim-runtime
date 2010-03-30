/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

import Qt 4.6

Item {
  property string titleText
  property string titleIcon
  property int handlePosition
  property alias content: contentArea.data
  property int handleWidth: 52
  property int handleHeight: 160
  property int dragThreshold: 16
  property int handleRadius: 12
  property int contentWidth: width - 2*handleWidth
  z:100

  Rectangle {
    id: background
    x: - handleWidth
    y: handlePosition
    width: 2*handleWidth
   height: handleHeight

    gradient: Gradient {
      GradientStop { position: 0.0; color: "lightgrey" }
      GradientStop { position: 0.5; color: "grey" }
    }
    effect: DropShadow {
      blurRadius: 8
      offset.x: 4
      offset.y: 4
    }
    radius: handleRadius

    Image {
      id: titleImage
      width: handleWidth - 4
      height: (titleIcon == '' ? 0 : handleWidth - 4)
      source: titleIcon
      anchors.bottom: parent.bottom
      anchors.right: parent.right
      anchors.margins: 2
    }

    Text {
      id: titleLabel
      anchors.left: parent.right
      anchors.bottom: titleImage.top
      width: parent.height - titleImage.height
      text: titleText
      transformOrigin: "BottomLeft"
      rotation: -90
      horizontalAlignment: "AlignHCenter"
      verticalAlignment: "AlignTop"
      font.bold: true
      font.pixelSize: 32
    }

    MouseArea {
      anchors.fill: parent
      drag.target: parent
      drag.axis: "XAxis"
      drag.minimumX: - handleWidth
      drag.maximumX: - handleWidth + dragThreshold + 1
    }

    Item {
      id: contentArea
      visible: false
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      width: contentWidth
    }

    states: [
      State {
        name: "expandedState"
        when: background.x >= (-handleWidth + dragThreshold)
        PropertyChanges {
          target: background
          height: background.parent.height - 40
          width: contentWidth + 2 * handleWidth
          y: 20
        }
        PropertyChanges { target: titleLabel; visible: false }
        PropertyChanges { target: titleImage; visible: false }
        PropertyChanges { target: contentArea; visible: true }
        PropertyChanges { target: background.parent; z: 50 }
      }
    ]

  }

}
