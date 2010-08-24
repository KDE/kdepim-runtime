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
import org.kde 4.5
import org.kde.akonadi 4.5

Item {
  id: wrapper
  clip: true

  property bool fullClickArea : false
  property bool showChildIndicator : false
  property bool selectedDelegate : false
  property bool topItem : false
  property bool showUnread : false
  property bool showCheckbox : false
  property bool checkable : false
  property bool uncheckable : false
  property bool alternatingRowColors : false
  property int indentation : 0
  property real dragCheckThreshold : 0.5

  property alias dragParent : dragTarget.parent

  property variant checkModel

  signal indexSelected(int row)

  x : indentation
  width : ListView.view.width - indentation

  Rectangle {
    // This is the same as anchors.fill : parent, but ParentAnimation only works
    // if positional layouting is used instead of anchor layouting.
    x : 0
    y : 0
    width : wrapper.width
    height : wrapper.height
    id : nestedItem
    color : ( alternatingRowColors && model.index % 2 == 0 ) ? "#33ffffff" : "#00000000"

    Behavior on x {
      id : dragFinishedBehavior
      SequentialAnimation {
        NumberAnimation {
          easing.type: "OutQuad"
          easing.amplitude: 100
          duration: 800
        }
        ScriptAction {
          script : {
            nestedItem.parent = wrapper
            nestedItem.y = 0
            dragFinishedBehavior.enabled = false
            nestedItem.x = 0
            dragFinishedBehavior.enabled = true
          }
        }
      }
    }

    MouseArea {
      anchors.fill: parent
      onClicked: {
        if ( fullClickArea )
        {
          if (topItem)
          {
            indexSelected(model.index);
            return;
          }

          if (showChildIndicator)
          {
            nestedItem.state = "before_select_child";
          } else if (!selectedDelegate)
            nestedItem.state = "before_select_breadcrumb";
          indexSelected(model.index);
        }
      }
      drag.target : (checkable || uncheckable) ? nestedItem : undefined
      drag.axis : Drag.XAxis
      drag.minimumX : uncheckable ? -nestedItem.width : wrapper.indentation
      drag.maximumX : checkable ? nestedItem.width : wrapper.indentation
      drag.onActiveChanged : {
        if (!drag.active) {
          if ((checkable && nestedItem.x > nestedItem.width * dragCheckThreshold)
            || (uncheckable && nestedItem.x < nestedItem.width * dragCheckThreshold))
          {
            // 8 is QItemSelectionModel::Toggle
            checkModel.select(model.index, 8);
          }
          nestedItem.x = wrapper.indentation
        } else {
          var point = mapToItem(dragParent, nestedItem.x, nestedItem.y)
          nestedItem.y = point.y
          dragFinishedBehavior.enabled = false;
          nestedItem.x = point.x + wrapper.indentation
          dragFinishedBehavior.enabled = true;
          nestedItem.parent = dragParent
          // Using the state directly does not seem to work.
//           nestedItem.state = "dragging"
        }
      }
    }
    Row {
      id: topLayout
      x: 10; y: 10;
      height: 48
      width: parent.width
      spacing: 10

      Rectangle {
        id : checkbox
        color : model.checkOn ? "blue" : "red"
        width : height
        height : 50
        visible : wrapper.showCheckbox;
      }

      //Image {
        //  id: collectionIcon
          // http://lists.trolltech.com/pipermail/qt-qml/2010-July/000668.html
  //        pixmap: KDE.iconToPixmap( model.decoration, height );
  //        width: 48; height: 48
//       }

      Column {
        height : parent.height
        Text {
          width: wrapper.width - 48 - 50
          text : model.display
          //### requires a newer QML version
          //wrapMode: "WrapAnywhere" // Needs the anchors.fill to work properly
        }

        Text {
          text : wrapper.showUnread && model.unreadCount > 0 ? KDE.i18n( "Unread: %1", model.unreadCount ) : ""
          color: "#0C55BB"
          font.pixelSize: 16
        }
      }
    }

    Image {
      width : height
      anchors.right : nestedItem.right
      anchors.rightMargin : 5
      anchors.verticalCenter : nestedItem.verticalCenter
      opacity : ( showChildIndicator && breadcrumbComponentFactory.childCollectionHasChildren( model.index ) ) ? 1 : 0
      source: "transparentplus.png"
    }

    states : [
      State {
        name : "dragging"
        ParentChange { id : dragTarget; target : nestedItem }
      },
      State {
        name : "before_select_child"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 35; y : 0 }
      },
      State {
        name : "before_select_breadcrumb"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 35; y : 0 }
      }
    ]
    transitions : [
      Transition {
        ParentAnimation {
          target : nestedItem
          NumberAnimation {
            properties: "x,y";
            duration : 500
            easing.type: Easing.OutQuad;
          }
        }
      }
    ]
  }
}

