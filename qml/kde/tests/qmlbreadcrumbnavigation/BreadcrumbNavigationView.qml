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

  property int itemHeight : height / 6
  property int _transitionSelect : -1

  signal childCollectionSelected(int row)
  signal breadcrumbCollectionSelected(int row)

  SystemPalette { id: palette; colorGroup: "Active" }
  ListModel {
    id : topModel
    ListElement { display : "Top" }
  }

  ListView {
    id : topButton
    property int _breadcrumb_y_offset : 0
    interactive : false
    height : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) itemHeight; else 0; }
    opacity : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) 1; else 0; }
    anchors.top : parent.top
    anchors.topMargin : _breadcrumb_y_offset
    anchors.left : parent.left
    anchors.right : parent.right
   model : topModel
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

  ListView {
    id : selectedItemView
    property int _selected_padding : 0

    height : itemHeight * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
    anchors.topMargin : _selected_padding
    anchors.left : parent.left
    anchors.right : parent.right
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
        _breadcrumb_y_offset : -1 * itemHeight
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
        target : topButton
        _breadcrumb_y_offset : { itemHeight * (2 - breadcrumbTopLevel._transitionSelect ) }
        opacity : 0.5
      }
      PropertyChanges {
        target : breadcrumbsView
        _breadcrumb_y_offset : { itemHeight * (2 - breadcrumbTopLevel._transitionSelect ) }
        height : { itemHeight * ( breadcrumbTopLevel._transitionSelect + 1 ) }
        opacity : 0.5
      }
    },
    State {
      name : "after_select_breadcrumb"
      PropertyChanges {
        target : childItemsView
      }
    }
  ]

  transitions : [
    Transition {
      from : ""
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
            properties: "_selected_padding"
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
        duration: 1000
        easing.type: "OutQuad"
        target: childItemsView
        properties: "opacity"
      }
    },
    Transition {
      from : ""
      to : "before_select_breadcrumb"
      SequentialAnimation {
        ParallelAnimation {
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: topButton
            properties: "height,_breadcrumb_y_offset,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: breadcrumbsView
            properties: "height,_breadcrumb_y_offset,opacity"
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
    }
  ]
}
