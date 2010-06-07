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
import org.qt 4.7 // Qt widget wrappers
import org.kde.pim.mobileui 4.5 as KPIM

Item {
  id : _topContext

  property alias styleSheet: columnView.styleSheet
  signal canceled()
  signal finished()

  QmlColumnView {
    id : columnView
    model : allFoldersModel
    selectionModel : folderSelectionModel
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right
    anchors.bottom: buttonBox.top
    anchors.topMargin : 30
  }

  Item {
    id: buttonBox
    anchors.bottom : parent.bottom
    anchors.right : parent.right
    anchors.left: parent.left
    height: 52 // 48  + 4 for margins

    KPIM.Button2 {
      anchors { left: parent.left; top: parent.top; bottom: parent.bottom; margins: 2 }
      width : 100

      buttonText : KDE.i18n("Cancel")
      onClicked : { canceled(); }
    }

    KPIM.Button2 {
      id: doneButton
      anchors { right: parent.right; top: parent.top; bottom: parent.bottom; margins: 2 }
      width : 100

      buttonText : KDE.i18n("Done")
      onClicked : { finished(); }
    }
  }
}
