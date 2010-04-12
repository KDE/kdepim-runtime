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

Item {
  id: breadcrumbTopLevel
  clip : true

  property alias breadcrumbItemsModel : breadcrumbsView.model
  property alias selectedItemModel : selectedItemView.model
  property alias childItemsModel : childItemsView.model

  property int itemHeight : 68
  property int _transitionSelect : -1

  signal childCollectionSelected(int row)
  signal breadcrumbCollectionSelected(int row)

  signal collectionSelected

  SystemPalette { id: palette; colorGroup: "Active" }

  Component {
    id : breadcrumbDelegate
    CollectionDelegate {
      fullClickArea : true
      steppedIndent : true
      onIndexSelected : {
        breadcrumbCollectionSelected(row);
      }
    }
  }

  Component {
    id : selectedItemDelegate
    CollectionDelegate {
      itemBackground : palette.dark
      indentOnly : true
    }
  }

  Component {
    id : childItemsDelegate
    property alias showChildIndicator : childDelegateWrapper.showChildIndicator
    property alias itemBackground : childDelegateWrapper.itemBackground
    CollectionDelegate {
      id : childDelegateWrapper
      fullClickArea : true
      showChildIndicator : true
      indentAll : true
      onIndexSelected : {
        breadcrumbTopLevel._transitionSelect = row;
        breadcrumbTopLevel.state = "before_select_child";
      }
    }
  }

  Rectangle {
    id : topButton
    height : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) itemHeight; else 0; }
    opacity : { if ( breadcrumbsView.count <= 1 && selectedItemView.count == 1 ) 1; else 0; }
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right

    Text { text : "Top" }

    MouseArea {
      anchors.fill : parent
      onClicked : {
        breadcrumbCollectionSelected(-1);
      }
    }
  }

  ListView {
    id : breadcrumbsView
    interactive : false
    property int selectedIndex : -1
    property int _breadcrumb_y_offset : 0
    height : { count = ( breadcrumbsView.count > 2 ) ? 2 : breadcrumbsView.count; itemHeight * count }
    anchors.top : topButton.bottom
    anchors.topMargin : _breadcrumb_y_offset
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : breadcrumbDelegate
  }

  ListView {
    id : selectedItemView
    property int _selected_padding : 0

    height : itemHeight * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
    anchors.topMargin : _selected_padding
    anchors.left : parent.left
    anchors.right : parent.right
    delegate : selectedItemDelegate
  }
  ListView {
    id : childItemsView
    property int _children_padding : 0
    property int _children_left_padding : 0
    property int _y_scroll : 0
    clip : true
    anchors.top : selectedItemView.bottom
    anchors.bottom : breadcrumbTopLevel.bottom
    anchors.bottomMargin : _children_padding
    anchors.left : parent.left
    anchors.leftMargin : _children_left_padding
    anchors.right : parent.right
    delegate : childItemsDelegate
    contentY : _y_scroll
  }

  function completeSelection() {
    childCollectionSelected(breadcrumbTopLevel._transitionSelect);
    breadcrumbTopLevel._transitionSelect = -1;
    breadcrumbTopLevel.state = "after_select_child";
    breadcrumbTopLevel.state = "";
  }

  states : [
    State {
      name : "before_select_child"
      PropertyChanges {
        target : breadcrumbsView
        height : { count = ( breadcrumbsView.count > 2 ) ? 2 : breadcrumbsView.count; itemHeight * ( count + 1) }
        _breadcrumb_y_offset : -1 * itemHeight
      }
      PropertyChanges {
        target : selectedItemView
        _selected_padding : -1 * itemHeight
      }
      PropertyChanges {
        target : childItemsView
        anchors.bottom : undefined
        _children_padding : childItemsView.height - itemHeight
        _y_scroll : itemHeight * breadcrumbTopLevel._transitionSelect
      }
      PropertyChanges {
        target : childItemsDelegate
        itemBackground : palette.dark
        indentAll : false
        indentOnly : true
        fullClickArea : false
        showChildIndicator : true
      }
    },
    State {
      name : "after_select_child"
      PropertyChanges {
        target : childItemsView
        _children_left_padding : breadcrumbTopLevel.width
      }
    },
    State {
      name : "before_select_breadcrumb"
      PropertyChanges {
        target : selectedItemView
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
            easing.type: "OutBounce"
            target: breadcrumbsView
            properties: "height,_breadcrumb_y_offset"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutBounce"
            target: selectedItemView
            properties: "_selected_padding"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutBounce"
            target: childItemsView
            properties: "_children_padding,_y_scroll,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutBounce"
            target: childItemsDelegate
            properties: "itemBackground"
          }
        }
        ScriptAction {
         script: { completeSelection(); }
       }
      }
    },
    Transition {
      from : "after_select_child"
      to : ""
      NumberAnimation {
        duration: 500
        easing.type: "OutBounce"
        target: childItemsView
        properties: "_children_left_padding"
      }
    }
  ]

}
