
import Qt 4.7

Rectangle {
  width : 800
  height : 480

  Rectangle {
    x : 30
    y : 30
    color : "lightsteelblue"
    width : 300
    height : 300

    Rectangle {
      color : "yellow"
      x : 50
      y : 50
        z : 1000
      width : 100
      height : 100


      MouseArea {
        anchors.fill : parent
        onClicked : {
          console.log("Clicked");
        }
        onPressed : {
          console.log("P")
          mouse.accepted = true
        }
        onPressAndHold : {
          console.log("P AND H")
          mouse.accepted = false
        }
      }

    }

    MouseArea {
      id: mrDrag
      anchors.fill: parent

      onClicked : {
        mouse.accepted = false;
        return;
      }

      onReleased: {
        if (!drag.active)
        {
          mouse.accepted = false;
          return;
        }
      }
      drag.target: parent;
      drag.axis: "XAxis"
      drag.minimumX: 30
      drag.maximumX: 130
    }
  }

}
