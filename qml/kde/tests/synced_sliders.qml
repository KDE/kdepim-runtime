
import Qt 4.7

Rectangle {
  width : 800
  height : 480

  property real syncedExtension

  SliderComponent {
    id : one
    y : 30
    x : 50
    height : 100
    leftBound : 30
    rightBound : 130
    threshold : 0.2
    expander : true

    onExtensionChanged : {
      two.changeExtension(extension)
    }
  }
  SliderComponent {
    id : two
    y : 130
    x : 50

    height : 100
    leftBound : 30
    rightBound : 430
    threshold : 0.7
    onExtensionChanged : {
      one.changeExtension(extension)
    }
  }

}
