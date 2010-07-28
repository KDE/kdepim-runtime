
import Qt 4.7

Rectangle {
  width : 800
  height : 480

  Grid {
    x : 30
    y : 30
    columns : 3
    spacing : 30
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.TopLeft
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Top
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.TopRight
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Left
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Center
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Right
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5
      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.BottomLeft
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : 30
        transformOrigin : Item.Bottom
      }
    }
    Rectangle {
      height : children[0].height
      width : children[0].width
      border.color: "lightsteelblue"
      border.width : 5

      Text {
        height : 20
        text : "Something"
        rotation : -90
        transformOrigin : Item.BottomRight
      }
    }
  }
}
