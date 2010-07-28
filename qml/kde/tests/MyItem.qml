
import Qt 4.7

Rectangle {
  width : 50
  height : 50

  color : "blue"

  signal triggered(variant triggeredObject)

  MouseArea {
    anchors.fill : parent
    onClicked : {
      parent.color = "green"
      console.log("CLICK TRIGGERED " + parent)
      triggered(parent)
    }
  }

}