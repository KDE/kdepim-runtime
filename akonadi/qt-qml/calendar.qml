import Qt 4.6
import "content"

Rectangle {
  width: 490; height: 400;

  //color: "black"
/*
  PortholeView {
    id : yearPorthole
    model : years_list
    height : 20
    anchors.top : parent.top
    anchors.left : parent.left; anchors.right : parent.right
  }

  PortholeView {
    id : monthsPorthole
    model : months_list
    height : 20
    anchors.top : yearPorthole.bottom
    anchors.left : parent.left; anchors.right : parent.right
  }
*/
  Component {
    id: yearDelegate
    Item {
      height : 15
      width : 15 + yearText.width
      Text { id: yearText; text: modelData }
    }
  }

  ListView {
    id : yearView
    anchors.top : parent.top
    height : 20
    anchors.left : parent.left; anchors.right : parent.right
    orientation : Qt.Horizontal
    model : years_list
    delegate : yearDelegate
  }

  Component {
    id: monthDelegate
    Item {
      height : 15
      width : 15 + monthText.width
      Text { id: monthText; text: modelData }
    }
  }

  ListView {
    id : monthView
    height : 20
    anchors.top : yearView.bottom
    anchors.left : parent.left; anchors.right : parent.right
    anchors.bottom : parent.bottom
    orientation : Qt.Horizontal
    model : months_list
    delegate : monthDelegate
  }

  DetailView {
    id : detail
  }

  CalendarView {
    id: view
    numDays : 30
    dayStartOffset : 5
    events_model : event_group_model

    onDateSelected : {
      detail.myData = eventGroup
      detail.state = 'visible';
    }
  }
}
