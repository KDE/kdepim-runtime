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
  property alias multipleSelectionText : multipleSelectionMessage.text

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
    interactive : false
    height : itemHeight

    anchors.top : parent.top

    anchors.left : parent.left
    anchors.right : parent.right
    model : topModel

    Image {
      id : topRightDivider
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
    height : breadcrumbsView.count > 0 ? itemHeight : 0

    clip : true;
    property int selectedIndex : -1
    anchors.top : topButton.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    highlightFollowsCurrentItem : true
    highlightRangeMode : ListView.StrictlyEnforceRange
    preferredHighlightBegin : 0
    preferredHighlightEnd : height
    onCountChanged : {
//       console.log("count ###" + count);
//       console.log(indexAt(0, 0) + " " + currentIndex);
//       positionViewAtIndex(count - 1, ListView.Beginning)
//       console.log("DONE" + indexAt(0, 0));
      //if (count > 0 )
//         contentY = itemHeight
    }
  }

  Item {
    id : breadcrumbPlaceHolder
    height : breadcrumbTopLevel.hasBreadcrumbs ? itemHeight : 0
    anchors.top : topButton.bottom
    anchors.left : parent.left
    anchors.right : parent.right
  }
  Image {
    id : breadcrumbRightDivider
    source : "dividing-line.png"
    anchors.top : breadcrumbPlaceHolder.top
    anchors.right : breadcrumbPlaceHolder.right
    height : breadcrumbTopLevel.hasBreadcrumbs ? (itemHeight -8) : 0
    fillMode : Image.TileVertically
    opacity : breadcrumbTopLevel.hasBreadcrumbs ? 1 : 0
  }

  ListView {
    id : selectedItemView
    interactive : false

    height : itemHeight * selectedItemView.count
    anchors.top : breadcrumbsView.bottom
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
    height : selectedItemView.count > 0 ? itemHeight : 0
    anchors.top : breadcrumbPlaceHolder.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    Item {
      id : selectedPlaceHolderImages
      anchors.fill : parent
      opacity : (selectedItemView.count > 0) ? 1 : 0
      Image {
        source : "dividing-line-horizontal.png"
        fillMode : Image.TileHorizontally
        anchors.top : parent.top
        anchors.topMargin : -3
        anchors.right : topLine.left
        anchors.left : parent.left
      }
      Image {
        id : topLine
        source : "list-line-top.png"
        anchors.right : parent.right
        anchors.top : parent.top
        anchors.topMargin : -8
      }
    }
  }

  ListView {
    id : childItemsView
    property bool shouldBeFlickable

    clip : true
    anchors.top : selectedItemPlaceHolder.bottom
    anchors.bottom : breadcrumbTopLevel.bottom
    anchors.left : parent.left
    anchors.right : parent.right

    shouldBeFlickable : {console.log("£SDFSDFSDF " + childItemsView.height + " " + count + " " + ( itemHeight * childItemsView.count)); childItemsView.height < (itemHeight * childItemsView.count) }
    interactive : shouldBeFlickable
  }

  Item {
    id : childItemsViewPlaceHolder
    anchors.top : selectedItemPlaceHolder.bottom
    anchors.bottom : breadcrumbTopLevel.bottom
    anchors.left : parent.left
    anchors.right : parent.right


    Image {
      source : "dividing-line-horizontal.png"
      fillMode : Image.TileHorizontally
      anchors.right : parent.right
      anchors.left : parent.left
      anchors.top : parent.top
    }

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
      opacity : {console.log("HEER" + childItemsView.shouldBeFlickable ); childItemsView.shouldBeFlickable ? 1 : 0 }
    }
    Image {
      source : "scrollable-bottom.png"
      anchors.bottom : parent.bottom
      anchors.right : parent.right
      anchors.left : parent.left
      fillMode : Image.TileHorizontally
      opacity : childItemsView.shouldBeFlickable ? 1 : 0
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
      id : multipleSelectionMessage
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

  function completeHomeSelection() {
    breadcrumbCollectionSelected(breadcrumbTopLevel._transitionSelect);
    breadcrumbTopLevel._transitionSelect = -1;
    breadcrumbTopLevel.state = "after_select_breadcrumb";
    breadcrumbTopLevel.state = "";
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
      name : "before_select_home"
      PropertyChanges {
        target : breadcrumbsView
        opacity : 0
        height : 0
      }
      PropertyChanges {
        target : breadcrumbPlaceHolder
        opacity : 0
        height : 0
      }
      PropertyChanges {
        target : selectedItemView
        opacity : 0
        height : 0
      }
      PropertyChanges {
        target : selectedItemPlaceHolder
        opacity : 0
        height : 0
      }
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
    },
    State {
      name : "before_select_child"
      PropertyChanges {
        target : topRightDivider
        anchors.bottomMargin : 0
        opacity : 1
      }
      PropertyChanges {
        target : breadcrumbsView
        height : itemHeight
        anchors.topMargin : -itemHeight
        opacity : 0
      }
      PropertyChanges {
        target : breadcrumbRightDivider
        anchors.topMargin : -8
        height : {console.log(itemHeight); 67}
        opacity : 0 // { 1 } // selectedItemView.count > 0 ? 1 : 0
      }
      PropertyChanges {
        target : selectedItemPlaceHolder
        anchors.topMargin : (breadcrumbsView.count == 0 && selectedItemView.count > 0) ? (itemHeight) : (breadcrumbsView.count == 0) ? 8 : 0
        height : itemHeight
        opacity : 1
      }

      PropertyChanges {
        target : selectedPlaceHolderImages
        opacity : 1
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
        height : { if (breadcrumbTopLevel._transitionSelect >= 0) itemHeight * ( breadcrumbTopLevel._transitionSelect + 1 ) }
        anchors.bottomMargin : -itemHeight
        opacity : 0.5
      }
      PropertyChanges {
        target : selectedItemView
        anchors.topMargin : (application.selectedCollectionRow() + breadcrumbsView.count) * itemHeight;
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
        target : breadcrumbsView
        contentY : breadcrumbsView.count > 1 ? itemHeight : 0
      }
      PropertyChanges {
        target : childItemsView
        opacity : 0
      }
    }
  ]

  transitions : [
    Transition {
      from : "*"
      to : "before_select_home"
      SequentialAnimation {
        ParallelAnimation {
          PropertyAnimation {
            target : breadcrumbPlaceHolder
            duration: 500
            easing.type: "OutQuad"
            properties : "opacity,height"
          }
          PropertyAnimation {
            target : breadcrumbsView
            duration: 500
            easing.type: "OutQuad"
            properties : "opacity,height"
          }
          PropertyAnimation {
            target : selectedItemView
            duration: 500
            easing.type: "OutQuad"
            properties : "opacity,height"
          }
          PropertyAnimation {
            target : selectedItemPlaceHolder
            duration: 500
            easing.type: "OutQuad"
            properties : "opacity,height"
          }
          PropertyAnimation {
            target : childItemsView
            duration: 500
            easing.type: "OutQuad"
            properties : "height"
          }
        }
        ScriptAction {
          script: { completeHomeSelection(); }
        }
      }
    },
    Transition {
      from : "*"
      to : "before_select_child"
      SequentialAnimation {
        ParallelAnimation {
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: topRightDivider
            properties: "opacity,anchors.bottomMargin"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: breadcrumbsView
            properties: "height,anchors.topMargin,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: breadcrumbRightDivider
            properties: "height"
          }
          PropertyAnimation {
            target : selectedItemPlaceHolder
            duration: 500
            easing.type: "OutQuad"
            properties : "anchors.topMargin,height,opacity"
          }
          PropertyAnimation {
            target : selectedPlaceHolderImages
            duration: 500
            easing.type: "OutQuad"
            properties : "opacity"
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
      ScriptAction {
        script : {console.log("### " + selectedItemView.count + " " + selectedItemView.currentIndex); }
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
            properties: "height,opacity"
          }
          PropertyAnimation {
            duration: 500
            easing.type: "OutQuad"
            target: selectedItemView
            properties: "opacity,anchors.topMargin"
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
