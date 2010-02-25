import Qt 4.6

Rectangle {
  id: calendarObject
  width: 490; height: 400;
  color: "black"

  property int numDays : 0
  property int dayStartOffset : 0
  property var events_model : 0
  property int month : 2
  property int year : 2010

  signal dateSelected(var eventGroup)

  ListModel {
    id : dayRow
    ListElement { name: "Mon" }
    ListElement { name: "Tue" }
    ListElement { name: "Wed" }
    ListElement { name: "Thur" }
    ListElement { name: "Fri" }
    ListElement { name: "Sat" }
    ListElement { name: "Sun" }
  }

  Component {
    id: appDelegate
    Item {
      id : wrapper
      focus : true
      Script {
        function startupFunction()
        {
          if (index < calendarObject.dayStartOffset)
          {
            wrapper.visible = false;
          }
        }
      }

      width: 70; height: 50

      Rectangle {
        id: background
        x: 1; y: 2; width: parent.width - 2; height: parent.height - 4
        color: "#FEFFEE"; border.color: "#FFBE4F"; radius: 5
      }

      Column {
        id: topLayout
        x: 10; y: 10;
        Text { id: _t; text: index + 1 - calendarObject.dayStartOffset }
        Text { text: ( model.eventsGroup.count > 0 ) ? model.eventsGroup.count + " events" : "" }
      }
      MouseRegion {
        id: pageMouse
        anchors.fill: parent
        onClicked: dateSelected( model.eventsGroup )
      }
      Component.onCompleted: startupFunction();
      Keys.onEnterPressed: dateSelected( model.eventsGroup )
      Keys.onReturnPressed: dateSelected( model.eventsGroup )
    }
  }

  Component {
    id: dayDelegate
    Item {
      id : wrapper
      width: 70; height: 50

      Rectangle {
        id: background
        x: 1; y: 2; width: parent.width - 2; height: parent.height - 4
        color: "#FEBBEE"; border.color: "#FFBE4F"; radius: 5
      }

      Column {
        id: topLayout
        x: 10; y: 10;
        Text { id: _t; text: name }
      }
    }
  }

  Component {
    id: appHighlight
    Rectangle {
      width: 45; height: 45; color: "white"
    }
  }

  GridView {
    id : dayList
    cellWidth: 70; cellHeight: 50
    height: cellHeight
    anchors.top : parent.top
    anchors.left : parent.left; anchors.right : parent.right
    model: dayRow
    delegate: dayDelegate
  }

  GridView {
    id : view
    focus: true
    cellWidth: 70; cellHeight: 50
    anchors.top : dayList.bottom
    anchors.bottom: parent.bottom;
    anchors.left : parent.left; anchors.right : parent.right
    model: events_model
    delegate: appDelegate
    highlight: appHighlight
    onCurrentIndexChanged: if (currentIndex < parent.dayStartOffset) currentIndex = parent.dayStartOffset
  }
}
