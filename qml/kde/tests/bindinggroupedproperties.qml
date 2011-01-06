import Qt 4.7

Item {
  width : 400
  height : 400
  Rectangle {
   id : rectY
   color : "yellow"
   width : maY.containsMouse ? 200 : 100
   height : 100
   MouseArea {
     id : maY
     hoverEnabled : true
     anchors.fill : parent
   }
  }
  Rectangle {
    id : rectR
    color : "red"
    width : 50
    height : 50
    //anchors.left : rectY.right
  }
  Binding {
    target : rectR
    property : "anchors.left"
    value : rectY.right
  }
}

