
import Qt 4.7

Rectangle {
  width : 200
  height : 200

  SomeComponent {
    x : 30
    y : 30
    width : 30
    height : 30
    id : blueRect
    color : "blue"

    Rectangle {
      x : 130
      y : 30
      width : 30
      height : 30
      color : "red"

      signal triggered()

      MouseArea {
        anchors.fill : parent
        onClicked : parent.triggered()
      }
    }
  }
}