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
import org.kde 4.5
import ".."

Rectangle {
  width: 800
  height: 480

  SlideoutPanelContainer {
    id: panelContainer
    anchors.fill: parent

    SlideoutPanel {
      id: folderPanel
      titleText: "Folders"
      handleHeight: 150
      content: [
        Rectangle {
          color: "blue"
          anchors.fill: parent
        }
      ]
    }

    SlideoutPanel {
      id: actionPanel
      titleText: "Actions"
      titleIcon: KDE.iconPath( "akonadi", 48 );
      handleHeight: 150
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
      handleHeight: parent.height - actionPanel.handleHeight - folderPanel.handleHeight - 2*panelContainer.margin
      contentWidth: 400
      content: [
        Rectangle {
          color: "green"
          anchors.fill: parent
        }
      ]
    }
  }
}
