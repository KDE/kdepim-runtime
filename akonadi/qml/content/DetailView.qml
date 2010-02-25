
import Qt 4.6

Rectangle {
  id : detail

  x: 10; z: 100
  width: view.width - 20
  anchors.fill: parent;
  anchors.topMargin: 10; anchors.bottomMargin: 10
  anchors.leftMargin: 10; anchors.rightMargin: 10
  opacity: 0
  color: "black"

  property var myData : 0

  Component {
    id : myDelegate
    Item {
      id: wrapper
      width: list.width
      property real detailsOpacity : 0
      height : 68

      Rectangle {
        id: background
        x: 1; y: 2; width: parent.width - 2; height: parent.height - 4
        color: "#FEFFEE"; border.color: "#FFBE4F"; radius: 5
      }
      MouseRegion {
          id: pageMouse
          anchors.fill: parent
          onClicked: wrapper.state = 'Details';
      }

      Row {
        id: topLayout
        x: 58; y: 10;
        height: 30;
        width: parent.width
        spacing: 10

        Column {
          height: 64; width: background.width - 48 - 20
          id : basicColumn

          spacing: 5
          Text {
            text : model.data.summary
            font.pointSize: 12; font.bold: true
          }
          Text {
            text : model.data.location
            wrap: true; width: parent.width
          }
        }
      }
      Item {
        id: details
        x: 10; width: parent.width-20
        anchors.top: topLayout.bottom; anchors.topMargin: 10

        anchors.bottom: parent.bottom; anchors.bottomMargin: 10
        opacity: wrapper.detailsOpacity

        Text {
          id: descriptionTitle
          text: "Description"; font.pointSize: 12; font.bold: true
        }
        Flickable {
          anchors.top: descriptionTitle.bottom; anchors.topMargin : 5
          anchors.left: parent.left; anchors.leftMargin: 40
          anchors.right: parent.right; anchors.rightMargin: 40
          anchors.bottom : parent.bottom
          viewportHeight : descriptionText.height
          clip : true
          Text { id: descriptionText; text: model.data.description; wrap : true; width : details.width }
        }
      }

      CloseButton {
        anchors.right: background.right; anchors.rightMargin: 5
        y: 10; opacity: wrapper.detailsOpacity
        text: "Close"; onClicked: wrapper.state = '';
      }

      states: State {
        name: "Details"
        PropertyChanges { target: wrapper; detailsOpacity: 1; x: 0 }
        PropertyChanges { target: wrapper; height: list.height }
        PropertyChanges { target: wrapper.ListView.view; explicit: true; viewportY: wrapper.y }
        PropertyChanges { target: wrapper.ListView.view; interactive: false }
        PropertyChanges { target: detailClose; opacity : 0 }
      }
    }
  }

  ListView {
    id: list
    anchors.fill: parent
    model : parent.myData
    delegate: myDelegate
  }

  CloseButton {
    id : detailClose
    anchors.right: parent.right; anchors.rightMargin: 5
    y: 10; opacity: parent.opacity
    text: "Close"; onClicked: parent.state = '';
  }

  states : State {
    name : "visible"
    PropertyChanges { target: detail; opacity: 1; x: 0 }
    PropertyChanges { target: view; }
  }
}
