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

  signal canceled()
  signal finished()

  QmlColumnView {
    id : columnView
    model : allFoldersModel
    selectionModel : folderSelectionModel
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right
    anchors.topMargin : 30
    height : parent.height * .85

  }
  Row {
    anchors.top : columnView.bottom
    anchors.bottom : parent.bottom
//    anchors.right : parent.right
    width: parent.width

    KPIM.Button {
      anchors.left : parent.parent.left
      y : 10
      height : parent.height
      width : 50

      buttonText : "Cancel"
      onClicked : { canceled(); }
    }

    KPIM.Button {
      id: doneButton
      anchors.right : parent.right
      y : 10
      height : parent.height
      width : 50

      buttonText : "Done"
      onClicked : { finished(); }
    }
  }
}
