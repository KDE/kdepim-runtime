import Qt 4.6
import "content"

Rectangle {
  id: page
  width: 400
  height: 240
  color: "black"

  Component {
    id: contactDelegate

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
        x: 10; y: 10;
        height: contactImage.height;
        width: parent.width
        spacing: 10

        Image {
            id: contactImage
            pixmap: model.data.image;
            width: 48; height: 48
        }

        Column {
          height: contactImage.height; width: background.width-contactImage.width-20
          spacing: 5
          Text {
            text : model.data.name
            font.pointSize: 12; font.bold: true
          }
          Text {
            text : model.data.email
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
            id: methodTitle
            text: "Address"; font.pointSize: 12; font.bold: true
            anchors.top: parent.top
        }
        Text {
          anchors.top: methodTitle.bottom; anchors.bottom: parent.bottom
          id: addressText; text: model.data.address; wrap: true; width: details.width
        }
        Text {
          id: phoneTitle
          text: "Phone number"; font.pointSize: 12; font.bold: true
          anchors.left: methodTitle.right; anchors.leftMargin: 40
        }
        Text {
          text: model.data.phone
          anchors.top: phoneTitle.bottom; anchors.left: methodTitle.right; anchors.leftMargin: 40
        }
      }

      CloseButton {
        anchors.right: background.right; anchors.rightMargin: 5
        y: 10; opacity: wrapper.detailsOpacity
        text: "Close"; onClicked: wrapper.state = '';
      }

      states: State {
        name: "Details"
        PropertyChanges { target: background; color: "white" }
        PropertyChanges { target: contactImage; width : undefined; height : undefined }
        PropertyChanges { target: wrapper; detailsOpacity: 1; x: 0 }
        PropertyChanges { target: wrapper; height: list.height }
        PropertyChanges { target: wrapper.ListView.view; explicit: true; viewportY: wrapper.y }
        PropertyChanges { target: wrapper.ListView.view; interactive: false }
      }
      transitions: Transition {
        ParallelAnimation {
          ColorAnimation { property: "color"; duration: 500 }
          NumberAnimation {
            duration: 300; matchProperties: "detailsOpacity,x,viewportY,height,width"
          }
        }
      }
    }
  }

  ListView
  {
    id: list
    anchors.fill: parent
    model: entity_tree_model
    delegate : contactDelegate
  }
}
