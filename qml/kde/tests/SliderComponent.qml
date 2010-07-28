
import Qt 4.7

Item {
  id : _topLevel
  property real leftBound
  property real rightBound
  property real threshold : 0.5
  property bool expander

  Rectangle {
    id : left
    width : 10
    height : _topLevel.height
    x : leftBound - width - 2
    y : _topLevel.y
    color : "red"
  }

  opacity : expander ? ( someRect.x - rightBound) / (leftBound - rightBound) : ( someRect.x - leftBound ) / (rightBound - leftBound)

  signal extensionChanged(real extension)

  function changeExtension(extension)
  {
    console.log("FOO " + someRect.x + " " + leftBound + " " + rightBound)
    if (someRect.x != leftBound && someRect.x != rightBound)
      return;
    console.log("Changed")
    someRect.x = ( extension * (rightBound - leftBound) ) + leftBound
  }

  Rectangle {
    id : someRect
    x : leftBound
    y : _topLevel.y
    color : "lightsteelblue"
    width : 100
    height : 100

    onXChanged : {
      if (x != leftBound && x != rightBound)
        return;

      extensionChanged( ( x - leftBound ) / ( rightBound - leftBound ) )
    }

    Behavior on x {
      NumberAnimation {
        easing.type: "OutQuad"
        easing.amplitude: 100
        duration: 300
      }
    }

    MouseArea {
      id: mrDrag
      //property alias active : drag.active
      anchors.fill: parent
      onReleased: {
        if (someRect.x > leftBound + ( ( rightBound - leftBound ) * threshold ) )
        {
          someRect.x = rightBound
        } else {
          someRect.x = leftBound
        }
      }
      drag.target: parent;
      drag.axis: "XAxis"
      drag.minimumX: leftBound
      drag.maximumX: rightBound
    }
  }

  Rectangle {
    id : right
    width : 10
    height : _topLevel.height
    x : rightBound + someRect.width + 2
    y : _topLevel.y
    color : "red"
  }

  Rectangle {
    width : 1
    height : 10
    x : leftBound + ( ( rightBound - leftBound ) * threshold )
    y : _topLevel.y + _topLevel.height
    color : "red"
  }
}
