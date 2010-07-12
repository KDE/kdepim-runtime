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

/** Akonadi Collection List View
    @param model: The collection model to display, ETM-based, filtered to only contain a flat collection list.
    @param currentIndex: Index of the currently selected row.
 */
Item {
  id: collectionListViewTopLevel
  property alias model: collectionListView.model
  property alias currentIndex: collectionListView.currentIndex
  signal collectionSelected

  SystemPalette { id: palette; colorGroup: "Active" }
  Component {
    id: collectionViewDelegate

    Item {
      id: wrapper
      width: collectionListView.width
      height : 68

      Rectangle {
        id: background
        opacity: 0.25
        x: 1; y: 2; width: parent.width - 2; height: parent.height - 4
        border.color: palette.mid
        radius: 5
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          wrapper.ListView.view.currentIndex = model.index;
          collectionListViewTopLevel.collectionSelected()
        }
      }

      Row {
        id: topLayout
        x: 10; y: 10;
        height: collectionIcon.height;
        width: parent.width
        spacing: 10

        Image {
            id: collectionIcon
            pixmap: KDE.iconToPixmap( model.decoration, height );
            width: 48; height: 48
        }

        Column {
          height: collectionIcon.height
          width: background.width - collectionIcon.width - 20
          spacing: 5
          Text {
            text : model.display
          }
        }
      }
    }
  }

  Component {
    id: highlight
    Rectangle {
      color: palette.highlight
      radius: 5
    }
  }

  ListView
  {
    id: collectionListView
    anchors.fill: parent
    delegate : collectionViewDelegate
    highlight: highlight
    highlightFollowsCurrentItem: true
    focus: true
    clip: true
  }
}
