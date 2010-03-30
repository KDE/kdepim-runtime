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
  property alias content: contentArea.data
  property int handlePosition: 0
  property int handleWidth: 36
  property int handleHeight: 160
  property int dragThreshold: 16
  property int handleRadius: 12
  property int contentWidth: width - handleWidth
  z:100

  function collapse() { background.x = -handleWidth }

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
    radius: handleRadius

    // don't use DropShadow effect, way to slow as it seems to affect everything in the content area as well
    Rectangle {
      color: "darkgrey"
      opacity: 0.75
      width: parent.width
      height: parent.height
      radius: handleRadius
      x: 4
      y: 4
      z: -1
      // WTF: Blur disables resizing!?!?
      effect: Blur { blurRadius: 4 }
    }

    Image {
      id: titleImage
      width: handleWidth - 4
      height: (titleIcon == '' ? 0 : handleWidth - 4)
      source: titleIcon
      anchors.bottom: (titleText == '' ? 0 : parent.bottom)
      anchors.verticalCenter: (titleText == '' ? parent.verticalCenter : 0)
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
          height: background.parent.height
          width: contentWidth + 2 * handleWidth - dragThreshold
          y: 0
        }
        PropertyChanges { target: titleLabel; visible: false }
        PropertyChanges { target: titleImage; visible: false }
        PropertyChanges { target: contentArea; visible: true }
        PropertyChanges { target: background.parent; z: 50 }
      }
    ]

  }

}
