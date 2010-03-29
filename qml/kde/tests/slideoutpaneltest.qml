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
import ".."

Rectangle {
  width: 800
  height: 480

  SlideoutPanel {
    anchors.fill: parent
    title: "Folders"
    handlePosition: 40
    contentData: [
      Rectangle {
        color: "blue"
        x: 10
        y: 20
        height: 100
        width: 100
      }
    ]
  }

  SlideoutPanel {
    anchors.fill:parent
    title: "Actions"
    handlePosition: 200
  }
}
