
import Qt 4.7

Rectangle {
  property alias text : _text.text;
  property bool clickable : false
  property bool isChild : false
  property bool isSelected : false
  property bool topItem : false

  signal indexSelected(int row)

  color : "#00000000"
  height: itemHeight;
  width : ListView.view.width

  Item {
    // This is the same as anchors.fill : parent, but ParentAnimation only works
    // if positional layouting is used instead of anchor layouting.
    x : 0
    y : 0
    width : parent.width
    height : parent.height
    id : nestedItem

    Text {
      id : _text
      anchors.verticalCenter : parent.verticalCenter;
      anchors.horizontalCenter : parent.horizontalCenter;
      text : model.display
    }

    Image {
      width : height
      anchors.right : parent.right
      anchors.rightMargin : 5
      anchors.verticalCenter : parent.verticalCenter
      opacity : ( isChild && application.childCollectionHasChildren( model.index ) ) ? 1 : 0
      source: "transparentplus.png"
    }

    MouseArea {
      anchors.fill : parent
      onClicked: {
        if ( clickable )
        {
          if (topItem)
          {
            indexSelected(model.index);
            return;
          }

          if (isChild)
          {
            nestedItem.state = "before_select_child";
          } else if (!isSelected)
            nestedItem.state = "before_select_breadcrumb";
          indexSelected(model.index);
        }
      }
    }

    states : [
      State {
        name : "before_select_child"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 0; y : 0 }
      },
      State {
        name : "before_select_breadcrumb"
        ParentChange { target : nestedItem; parent : selectedItemPlaceHolder; }
        PropertyChanges { target : nestedItem; x : 0; y : 0 }
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