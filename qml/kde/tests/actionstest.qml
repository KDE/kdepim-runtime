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

import Qt 4.7 as QML
import org.kde.pim.mobileui 4.5 as KPIM

QML.Rectangle {
  width : 800
  height : 500

  KPIM.ActionMenuContainer {
      x : 100
      y : 100
      width : 600
      height : 300
      id :myTree
      actionItemHeight : 70
      actionItemWidth : 200
      actionItemSpacing : 2

      FakeAction { name : "quit" }
      KPIM.ActionList {
        name : "file"
        KPIM.ActionList {
          name : "new"
          FakeAction { name : "new event" }
          FakeAction { name : "new alarm" }
          FakeAction { name : "new birthday" }

        }
        FakeAction { name : "import" }
        FakeAction { name : "export" }
      }

      KPIM.ActionList {
        name : "edit"
        FakeAction { name : "copy" }
        FakeAction { name : "move" }
        FakeAction { name : "cut" }

      }

      KPIM.ActionList {
        name : "view"
        FakeAction { name : "month_view" }
        FakeAction { name : "day_view" }
        FakeAction { name : "week_view" }
        FakeAction { name : "hour_view" }

      }

  /*
      KPIM.ActionList {
        name : "view"
        KPIM.ActionListItem { name : "month" }
        KPIM.ActionListItem { name : "week" }
        KPIM.ActionListItem { name : "day" }
      } */
      /*
      ActionList {
        name : "view"
        Action { "A" }
        ActionList {
          name : "B"
          Action { "B1" }
          Action { "B2" }
        }
        Action {
          Action { "C" }
        }
      } */
  }

}
