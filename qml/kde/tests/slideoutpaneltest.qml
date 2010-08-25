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

import Qt 4.7
import org.kde 4.5

Rectangle {
  width: 800
  height: 480

  color : "#00000000"

  SlideoutPanelContainer {
    id: panelContainer
    anchors.fill: parent

    SlideoutPanel {
      id: folderPanel
      titleText: KDE.i18n( "Folders" )
      handleHeight: 150
      content: [
        Rectangle {
          color: "blue"
          anchors.fill: parent

          Rectangle {
            color : "yellow"
            x : 300
            y : 100
            width : 100
            height : 100
            MouseArea {
              anchors.fill : parent
              onClicked : {
                console.log("Clicked!");
              }
            }
          }
        }
      ]
    }

    SlideoutPanel {
      id: actionPanel
      titleText: KDE.i18n( "Actions" )
      titleIcon: KDE.iconPath( "akonadi", 48 );
      handleHeight: 150
      collapsedPosition : 150
      expandedPosition : 25
      contentWidth: 200
      content: [
        Rectangle {
          color: "red"
          anchors.fill: parent
        }
      ]
    }

    SlideoutPanel {
      id: attachmentPanel
      titleIcon: KDE.iconPath( "mail-attachment", 48 );
      collapsedPosition : 300
      expandedPosition : 50
      contentWidth: 100
      content: [
        Rectangle {
          color: "green"
          anchors.fill: parent
        }
      ]
    }
  }
}
