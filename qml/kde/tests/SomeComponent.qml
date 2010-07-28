
import Qt 4.7

Rectangle {
  x : 30
  y : 30
  width : 30
  height : 30
  id : blueRect
  color : "blue"

  function mySlot()
  {
    console.log("Invoked");
  }

  function makeConnections()
  {
    for ( var i = 0; i < children.length; ++i ) {
      children[i].triggered.disconnect(this, mySlot)
      children[i].triggered.connect(this, mySlot)
    }
  }

  onChildrenChanged : makeConnections()
  Component.onCompleted : makeConnections()
}