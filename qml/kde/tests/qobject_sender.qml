
import Qt 4.7

Rectangle {
  width : 500
  height : 500

  function resetOthersColors(clickedObject)
  {
    console.log("CLICKED OBJECT " + clickedObject)
    for (var i = 0; i < children.length; ++i)
    {
      console.log("CHILD OBJECT " + children[i])
      if (children[i] == clickedObject)
        continue;
      children[i].color = "blue"
    }
  }

  MyItem { x: 100; y : 20 }
  MyItem { x: 100; y : 80 }
  MyItem { x: 100; y : 140 }

  Component.onCompleted : {
    for (var i = 0; i < children.length; ++i)
    {
      children[i].triggered.connect(this, resetOthersColors)
    }
  }
}