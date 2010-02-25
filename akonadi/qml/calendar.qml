import Qt 4.6
import "content"

Rectangle {
  width: 490; height: 400;
  color: "black"

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
