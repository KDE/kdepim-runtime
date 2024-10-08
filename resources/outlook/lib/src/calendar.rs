//! Module that implements traits and types to represent Calendaring API.
use std::{default, future::Future};

use chrono::{DateTime, NaiveDate, NaiveDateTime, Utc};
use graph_rs_sdk::GraphResult;
use icalendar::{
    Calendar as ICalCalendar, Class, Component, DatePerhapsTime, Event as ICalEvent, EventLike,
    EventStatus, Property,
};
use serde::{Deserialize, Serialize};
use serde_with::{serde_as, skip_serializing_none, NoneAsEmptyString};

pub struct EventsListResponse {
    pub events: Vec<Event>,
    pub delta_link: Option<String>,
}

impl From<(Vec<Event>, Option<String>)> for EventsListResponse {
    fn from((events, delta_link): (Vec<Event>, Option<String>)) -> Self {
        Self { events, delta_link }
    }
}

/// Trait to perform various basic operations on the calendar.
pub trait CalendarAPI {
    /// Returns a list of calendars
    fn list_calendars(&self) -> impl Future<Output = GraphResult<Vec<Calendar>>> + Send;

    /// Creates a new calendar
    fn create_calendar(
        &self,
        calendar: Calendar,
    ) -> impl Future<Output = GraphResult<Calendar>> + Send;

    /// Updates an existing calendar
    fn update_calendar(
        &self,
        calendar: Calendar,
    ) -> impl Future<Output = GraphResult<Calendar>> + Send;

    /// Deletes a calendar
    fn delete_calendar<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
    ) -> impl Future<Output = GraphResult<()>> + Send;

    /// Returns a list of events
    fn list_events<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
    ) -> impl Future<Output = GraphResult<EventsListResponse>> + Send;
    fn list_events_ids<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
    ) -> impl Future<Output = GraphResult<Vec<String>>> + Send;
    fn list_events_delta<ID: AsRef<str> + Send, Token: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        delta_token: Token,
    ) -> impl Future<Output = GraphResult<EventsListResponse>> + Send;
    fn list_events_changed_since<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        changed_since: DateTime<Utc>,
    ) -> impl Future<Output = GraphResult<Vec<Event>>> + Send;

    /// Creates a new event
    fn create_event<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        event: Event,
    ) -> impl Future<Output = GraphResult<Event>> + Send;

    /// Updates an existing event
    fn update_event<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        event: Event,
    ) -> impl Future<Output = GraphResult<Event>> + Send;

    /// Deletes an event
    fn delete_event<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        event_id: ID,
    ) -> impl Future<Output = GraphResult<()>> + Send;
}

/// Pre-defined set of color themes for calendars
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(rename_all = "camelCase")]
pub enum CalendarColor {
    Auto,
    LightBlue,
    LightGreen,
    LightOrange,
    LightGray,
    LightYellow,
    LightTeal,
    LightPink,
    LightBrown,
    LightRed,
    MaxColor,
}

fn mk_false() -> bool { false }

/// Represents a single calendar
#[serde_as]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Calendar {
    /// Globally unique calendar ID
    pub id: String,

    /// Identifies the version of the calendar object. Every time the calendar is changed, changeKey changes as well.
    pub change_key: String,

    /// `true` if the user can write to the calendar, `false` otherwise
    pub can_edit: bool,

    /// Specifies the color theme of the calendar to distinguish it from others
    pub color: CalendarColor,

    /// The calendar color, expressed in a hex color code.
    /// If the user has never explicitly set a color for the calendar, this property is empty.
    /// Read-only.
    #[serde_as(as = "NoneAsEmptyString")]
    pub hex_color: Option<String>,

    /// Name of the calendar
    pub name: String,

    /// Whether this is the user's default calendar
    #[serde(default = "mk_false")]
    pub is_default_calendar: bool,
}

/// Represents a single event in a calendar
#[serde_as]
#[skip_serializing_none]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Event {
    /// Collection of attendees for the event
    pub attendees: Option<Vec<Attendee>>,
    /// The body of the message associated with the event
    pub body: ItemBody,
    /// Categories associated with the event
    pub categories: Option<Vec<String>>,
    /// Identifies the version of the event object.
    /// Every time the event is changed, ChangeKey changes as well
    #[serde(skip_serializing)]
    pub change_key: String,
    /// Timestamp of when the event was created
    #[serde(skip_serializing)]
    pub created_date_time: DateTime<Utc>,
    /// The date, time, and time zone that the event ends.
    pub end: DateTimeTimeZone,
    /// A unique identifier for an event across calendars.
    #[serde(rename = "iCalUId", skip_serializing)]
    pub ical_uid: String,
    /// Unique identifier for the event.
    pub id: String,
    /// The importance of the event.
    pub importance: Option<Importance>,
    /// Set to true if the event lasts all day.
    pub is_all_day: Option<bool>,
    /// Set to true if the event has been canceled.
    pub is_cancelled: Option<bool>,
    /// Set to true if the calendar owner is the organizer of the event.
    pub is_organizer: Option<bool>,
    /// Set to true if the event has a reminder set.
    pub is_reminder_on: Option<bool>,
    /// Timestamp of when the event was last modified.
    pub last_modified_date_time: Option<String>,
    /// Location of the event.
    pub location: Option<Location>,
    /// The organizer of the event.
    pub organizer: Recipient,
    /// The timezone that was used when the event was created.
    pub original_end_time_zone: Option<String>,
    /// Represents the start time of an event when it is initially created as an occurrence or exception in a recurring series
    #[serde(skip_serializing)]
    pub original_start: Option<DateTime<Utc>>,
    /// The start time zone that was set when the event was created.
    pub original_start_timezone: Option<String>,
    /// The recurrence pattern for the event.
    pub recurrence: Option<PatternedRecurrence>,
    /// The number of minutes before the event start time that the reminder alert occurs.
    pub reminder_minutes_before_start: Option<i32>,
    /// Indicates the type of response sent in response to an event message.
    pub response_requested: Option<bool>,
    /// The response status of the event.
    pub response_status: Option<ResponseStatus>,
    /// sensitivity of the event
    pub sensitivity: Option<Sensitivity>,
    /// The ID for the recurring series master item, if this event is part of a recurring series.
    pub series_master_id: Option<String>,
    /// The status to show.
    pub show_as: Option<ShowAs>,
    /// The start date, time, and time zone of the event.
    pub start: DateTimeTimeZone,
    /// The text of the event's subject line.
    pub subject: String,
    /// The event type.
    #[serde(rename = "type")]
    pub type_: Option<EventType>,
    /// Web link
    pub web_link: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum Sensitivity {
    Normal,
    Personal,
    Private,
    Confidential,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum EventType {
    SingleInstance,
    Occurence,
    Exception,
    SeriesMaster,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ItemBody {
    pub content: String,
    pub content_type: BodyType,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum BodyType {
    Text,
    Html,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum ShowAs {
    Free,
    Tentative,
    Busy,
    #[serde(rename = "oof")]
    OOF,
    WorkingElsewhere,
    Unknown,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DateTimeTimeZone {
    pub date_time: NaiveDateTime,
    pub time_zone: String,
}

impl DateTimeTimeZone {
    fn into_maybe_allday(&self, all_day: &Option<bool>) -> DatePerhapsTime {
        match all_day {
            Some(true) => DatePerhapsTime::Date(self.date_time.date()),
            _ => DatePerhapsTime::DateTime(icalendar::CalendarDateTime::WithTimezone {
                date_time: self.date_time,
                tzid: self.time_zone.clone(),
            }),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum Importance {
    Low,
    Normal,
    High,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum AttendeePresenceType {
    Required,
    Optional,
    Resources,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum Response {
    None,
    Organizer,
    TentativelyAccepted,
    Accepted,
    Declined,
    NotResponded,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ResponseStatus {
    response: Response,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EmailAddress {
    pub address: String,
    pub name: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Attendee {
    pub email_address: EmailAddress,
    pub status: ResponseStatus,
    pub type_: AttendeePresenceType,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Location {
    pub address: Option<PhysicalAddress>,
    pub coordinates: Option<GeoCoordinates>,
    pub display_name: String,
    pub location_email_address: Option<String>,
    pub location_uri: Option<String>,
    #[serde(skip_serializing)]
    pub location_type: LocationType,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PhysicalAddress {
    pub city: Option<String>,
    pub country_or_region: Option<String>,
    pub postal_code: Option<String>,
    pub state: Option<String>,
    pub street: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GeoCoordinates {
    pub accuracy: Option<f64>,
    pub altitude: Option<f64>,
    pub altitude_accuracy: Option<f64>,
    pub latitude: Option<f64>,
    pub longitude: Option<f64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum LocationType {
    Default,
    ConferenceRoom,
    HomeAddress,
    BusinessAddress,
    GeoCoordinates,
    StreetAddress,
    Hotel,
    Restaurant,
    LocalBusiness,
    PostalAddress,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Recipient {
    pub email_address: EmailAddress,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PatternedRecurrence {
    pub pattern: RecurrencePattern,
    pub range: RecurrenceRange,
}

#[skip_serializing_none]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RecurrencePattern {
    pub day_of_month: Option<i32>,
    pub days_of_week: Option<Vec<DayOfWeek>>,
    pub first_day_of_week: Option<DayOfWeek>,
    pub index: Option<WeekIndex>,
    pub interval: i32,
    pub month: i32,
    #[serde(rename = "type")]
    pub type_: RecurrencePatternType,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum DayOfWeek {
    Sunday,
    Monday,
    Tueday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum WeekIndex {
    First,
    Second,
    Third,
    Fourth,
    Last,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum RecurrencePatternType {
    Daily,
    Weekly,
    AbsoluteMonthly,
    RelativeMonthly,
    AbsoluteYearly,
    RelativeYearly,
}

#[serde_as]
#[skip_serializing_none]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RecurrenceRange {
    pub end_date: Option<NaiveDate>,
    pub number_of_occurrences: Option<i32>,
    pub recurrence_timezone: Option<String>,
    pub start_date: NaiveDate,
    #[serde(rename = "type")]
    pub type_: RecurrenceRangeType,
}

/// The recurrence range
///
/// See [Graph Reference](https://learn.microsoft.com/en-us/graph/api/resources/recurrencerange?view=graph-rest-1.0)
/// for details.n""
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum RecurrenceRangeType {
    EndDate,
    NoEnd,
    Numbered,
}


impl From<&Event> for Vec<u8> {
    fn from(event: &Event) -> Self {
        let mut ical_event = ICalEvent::new()
            .uid(&event.ical_uid)
            .summary(&event.subject)
            .description(&event.body.content)
            .starts(event.start.into_maybe_allday(&event.is_all_day))
            .ends(event.end.into_maybe_allday(&event.is_all_day))
            .done();

        let organizer = &event.organizer.email_address;
        ical_event.append_property(
            Property::new("ORGANIZER".to_string(), format!("mailto:{}", &organizer.address))
                .add_parameter("NAME", &organizer.name)
                .done());
        if let Some(location) = &event.location {
            ical_event.location(&location.display_name);
        }
        if let Some(web_link) = &event.web_link {
            ical_event.url(&web_link);
        }
        if let Some(sensitivity) = &event.sensitivity {
            ical_event.class(match sensitivity {
                Sensitivity::Private => Class::Private,
                Sensitivity::Personal => Class::Private,
                Sensitivity::Confidential => Class::Confidential,
                Sensitivity::Normal => Class::Public,
            });
        }
        if let Some(importance) = &event.importance {
            ical_event.priority(match importance {
                Importance::Low => 9,
                Importance::Normal => 5,
                Importance::High => 1
            });
        }
        if let Some(status) = &event.response_status {
            match status.response {
                Response::Accepted => { ical_event.status(EventStatus::Confirmed); },
                Response::TentativelyAccepted => { ical_event.status(EventStatus::Tentative); },
                Response::Declined => { ical_event.status(EventStatus::Cancelled); },
                Response::Organizer => {}, // unmapped
                Response::NotResponded => {}, // unmapped
                Response::None => {}, // unmapped
            };
        }

        // FIXME: Add support for recurrence, reminders, ..


        format!("{}", ICalCalendar::new().push(ical_event.done()).done()).into()
    }
}
