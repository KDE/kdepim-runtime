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

Item {
  id: breadcrumbTopLevel
  clip : true

  property alias breadcrumbItemsModel : breadcrumbsView.model
  property alias selectedItemModel : selectedItemView.model
  property alias childItemsModel : childItemsView.model

  property alias topDelegate :  topButton.delegate
  property alias breadcrumbDelegate :  breadcrumbsView.delegate
  property alias selectedItemDelegate :  selectedItemView.delegate
  property alias childItemsDelegate :  childItemsView.delegate

  property int itemHeight : height / 7
  property int _transitionSelect : -1

  property bool hasChildren :  childItemsView.count > 0
  property bool hasSelection :  selectedItemView.count > 0
  property bool hasBreadcrumbs :  breadcrumbsView.count > 0

  property alias numSelected : selectedItemView.count

  signal childCollectionSelected(int row)
  signal breadcrumbCollectionSelected(int row)

  SystemPalette { id: palette; colorGroup: "Active" }
  ListModel {
    id : topModel
    ListElement { display : "Home" }
  }

  ListView {
    id : topButton
    property int _breadcrumb_y_offset : 0
    interactive : false
    height : itemHeight

    anchors.top : parent.top

    anchors.left : parent.left
    anchors.right : parent.right
    model : topModel

    Image {
      source : "dividing-line.png"
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.bottom : parent.bottom
      anchors.bottomMargin : breadcrumbTopLevel.hasBreadcrumbs ? 0 : 8
      fillMode : Image.TileVertically
      opacity : breadcrumbTopLevel.hasSelection ? 1 : 0
    }
  }

  ListView {
    id : breadcrumbsView
    interactive : false
    property int selectedIndex : -1
    property int _breadcrumb_y_offset : 0
    height : { ( ( breadcrumbsView.count > 2 ) ? 2 : breadcrumbsView.count ) * itemHeight }
    anchors.top : topButton.bottom
    anchors.topMargin : _breadcrumb_y_offset
    anchors.left : parent.left
    anchors.right : parent.right
  }

  Item {
    id : breadcrumbPlaceHolder
    height : itemHeight
    anchors.top : topButton.bottom
    anchors.left : parent.left
    anchors.right : parent.right

    Image {
      source : "dividing-line.png"
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.bottom : parent.bottom
      anchors.bottomMargin : 8
      fillMode : Image.TileVertically
    }
  }

  ListView {
    id : selectedItemView
    interactive : false
    property int _selected_padding : 0

    height : itemHeight * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
    anchors.topMargin : _selected_padding
    anchors.left : parent.left
    anchors.right : parent.right

    onCountChanged : {
      if (selectedItemView.count > 1)
      {
        breadcrumbsView.visible = false;
        breadcrumbPlaceHolder.visible = false;
        selectedItemView.visible = false;
        selectedItemPlaceHolder.visible = false;
        childItemsView.visible = false;
        favinfoOverlay.visible = true;
      }
      else
      {
        breadcrumbsView.visible = true;
        breadcrumbPlaceHolder.visible = true;
        selectedItemView.visible = true;
        selectedItemPlaceHolder.visible = true;
        childItemsView.visible = true;
        favinfoOverlay.visible = false;
      }
    }
  }

  Item {
    id : selectedItemPlaceHolder
    height : itemHeight
    anchors.top : breadcrumbPlaceHolder.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    Image {
      source : "dividing-line-horizontal.png"
      fillMode : Image.TileHorizontally
      anchors.top : parent.top
      anchors.topMargin : -3
      anchors.right : topLine.left
      anchors.left : parent.left
      opacity : (selectedItemView.count > 0) ? 1 : 0
    }
    Image {
      id : topLine
      source : "list-line-top.png"
      anchors.right : parent.right
      anchors.top : parent.top
      anchors.topMargin : -8
      opacity : (selectedItemView.count > 0) ? 1 : 0
    }
    Image {
      source : "dividing-line-horizontal.png"
      fillMode : Image.TileHorizontally
      anchors.right : parent.right
      anchors.left : parent.left
      anchors.bottom : parent.bottom
    }
  }

  ListView {
    id : childItemsView
    property int _children_padding : 0
    property int _y_scroll : 0
    clip : true
    anchors.top : selectedItemView.bottom
    anchors.bottom : breadcrumbTopLevel.bottom
    anchors.bottomMargin : _children_padding
    anchors.left : parent.left
    anchors.right : parent.right
    contentY : _y_scroll

    Image {
      source : "dividing-line.png"
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.bottom : parent.bottom
      fillMode : Image.TileVertically
    }
    Image {
      source : "scrollable-top.png"
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.left : parent.left
      fillMode : Image.TileHorizontally
    }
    Image {
      source : "scrollable-bottom.png"
      anchors.bottom : parent.bottom
      anchors.right : parent.right
      anchors.left : parent.left
      fillMode : Image.TileHorizontally
    }
  }

  Item {
    id : favinfoOverlay
    anchors.top : topButton.bottom
    anchors.bottom : parent.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    visible : false

    Text {
      text : KDE.i18na("You have selected \n%1 folders\nfrom N accounts\n%2 emails", [selectedItemView.count, headerList.count])
      font.italic : true
      horizontalAlignment : Text.AlignHCenter
      anchors.horizontalCenter : parent.horizontalCenter

      height : 30
      x : 20
      y : 50
    }

    Image {
      source : "dividing-line-horizontal.png"
      fillMode : Image.TileHorizontally
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.left : parent.left
    }

    Image {
      source : "dividing-line.png"
      fillMode : Image.TileVertically
      anchors.top : parent.top
      anchors.right : parent.right
      anchors.bottom : parent.bottom
    }

  }

  function completeChildSelection() {
    childCollectionSelected(breadcrumbTopLevel._transitionSelect);
    breadcrumbTopLevel._transitionSelect = -1;
    breadcrumbTopLevel.state = "after_select_child";
    breadcrumbTopLevel.state = "";
  }

  function completeBreadcrumbSelection() {
    breadcrumbCollectionSelected(breadcrumbTopLevel._transitionSelect);
    breadcrumbTopLevel._transitionSelect = -1;
    breadcrumbTopLevel.state = "after_select_breadcrumb";
    breadcrumbTopLevel.state = "";
  }

  states : [
    State {
      name : "before_select_child"
      PropertyChanges {
        target : breadcrumbsView
        height : { var _count = ( breadcrumbsView.count > 2 ) ? 2 : breadcrumbsView.count; itemHeight * ( _count + 1) }
        _breadcrumb_y_offset : { if (breadcrumbsView.count > 1) -1 * itemHeight }
      }
      PropertyChanges {
        target : selectedItemView
        _selected_padding : -1 * itemHeight
      }
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
    },
    State {
      name : "after_select_child"
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
    },
    State {
      name : "before_select_breadcrumb"
      PropertyChanges {
        target : breadcrumbsView
        _breadcrumb_y_offset : {
          if (breadcrumbTopLevel._transitionSelect >= 0)
            /* return */ itemHeight * (2 - breadcrumbTopLevel._transitionSelect )

        }

        height : { if (breadcrumbTopLevel._transitionSelect >= 0) itemHeight * ( breadcrumbTopLevel._transitionSelect + 1 ) }
        opacity : 0.5
      }
      PropertyChanges {
        target : selectedItemView
        opacity : 0
      }
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
    },
    State {
      name : "after_select_breadcrumb"
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
      PropertyChanges {
        target : selectedItemView
        opacity : 0
      }
    }
  ]

  transitions : [
    Transition {
      from : "*"
      to : "before_select_child"
      SequentialAnimation {
        ParallelAnimation {
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: breadcrumbsView
            properties: "height,_breadcrumb_y_offset"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: selectedItemView
            properties: "_selected_padding,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: childItemsView
            properties: "opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: childItemsDelegate
            properties: "itemBackground"
          }
        }
        ScriptAction {
         script: { completeChildSelection(); }
        }
      }
    },
    Transition {
      from : "after_select_child"
      to : ""
      NumberAnimation {
        target: childItemsView
        properties: "opacity"
      }
      NumberAnimation {
        target: selectedItemView
        properties: "opacity"
      }
    },
    Transition {
      from : "*"
      to : "before_select_breadcrumb"
      SequentialAnimation {
        ParallelAnimation {
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: breadcrumbsView
            properties: "height,_breadcrumb_y_offset,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: selectedItemView
            properties: "opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: childItemsView
            properties: "opacity"
          }
        }
        ScriptAction {
         script: { completeBreadcrumbSelection(); }
        }
      }
    },
    Transition {
      from : "after_select_breadcrumb"
      to : ""
      NumberAnimation {
        duration: 500
        easing.type: "OutQuad"
        target: childItemsView
        properties: "opacity"
      }
      NumberAnimation {
        duration: 500
        easing.type: "OutQuad"
        target: selectedItemView
        properties: "opacity"
      }
    }
  ]
}
