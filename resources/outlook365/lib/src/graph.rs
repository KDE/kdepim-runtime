use chrono::{DateTime, Utc};
use futures_core::Stream;
use graph_http::traits::ODataDeltaLink;
use graph_rs_sdk::{error::ErrorMessage, GraphClient, GraphClientConfiguration, GraphFailure, GraphResult, ODataQuery};
use http::Response;
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use serde_json::Value;
use test_context::futures::StreamExt;
use tokio::sync::Mutex;
use url::Url;

use crate::calendar::{Calendar, CalendarAPI, Event, EventsListResponse};
pub struct Resource {
    client: Mutex<GraphClient>,
    min_sync_date: Option<DateTime<Utc>>,
}

impl Resource {
    pub fn new(access_token: String) -> Self {
        Self {
            client: Mutex::new(GraphClient::new(access_token)),
            min_sync_date: None,
        }
    }

    pub fn new_with_configuration(configuration: GraphClientConfiguration) -> Self {
        Self {
            client: Mutex::new(GraphClient::from(configuration)),
            min_sync_date: None,
        }
    }

    #[cfg(test)]
    pub async fn use_test_endpoint(self, endpoint: &Url) -> Self {
        self.client.lock().await.use_test_endpoint(endpoint);
        self
    }

    pub async fn sync_after(&mut self, date: DateTime<Utc>) {
        self.min_sync_date = Some(date);
    }
}

#[derive(Debug, Clone, Deserialize, Serialize)]
struct GraphResponse<T> {
    value: T,
}

impl Resource {
    async fn extract_paged_delta_result<T>(&self, mut stream: Box<impl Stream<Item = GraphResult<Response<Result<Value, ErrorMessage>>>> + Unpin>) -> GraphResult<(Vec<T>, Option<String>)>
    where
        T: DeserializeOwned
    {
        let mut events =  Vec::<T>::new();
        let mut delta_token: Option<String> = None;
        while let Some(result) = stream.next().await {
            let response = result?;
            let body = response.into_body()?;
    
            if let Some(url) = body.odata_delta_link() {
                if let Some((_, token)) = Url::parse(&url)?.query_pairs().find(|(key, _)| key == "$deltatoken") {
                    delta_token = Some(token.into_owned());
                }
            }
    
            let results: GraphResponse<Vec<T>> = serde_json::from_value(body)?;
            events.extend(results.value);
        }
    
        Ok(( events, delta_token ))
    }
}

impl CalendarAPI for Resource {
    async fn list_calendars(&self) -> GraphResult<Vec<Calendar>> {
        Ok(self
            .client
            .lock()
            .await
            .me()
            .calendars()
            .list_calendars()
            .send()
            .await?
            .json::<GraphResponse<Vec<Calendar>>>()
            .await
            .map_err(GraphFailure::ReqwestError)?
            .value)
    }

    async fn create_calendar(&self, calendar: Calendar) -> GraphResult<Calendar> {
        self.client
            .lock()
            .await
            .me()
            .calendars()
            .create_calendars(&calendar)
            .send()
            .await?
            .json()
            .await
            .map_err(GraphFailure::ReqwestError)
    }

    async fn update_calendar(&self, calendar: Calendar) -> GraphResult<Calendar> {
        self.client
            .lock()
            .await
            .me()
            .calendar(&calendar.id)
            .update_calendars(&calendar)
            .send()
            .await?
            .json()
            .await
            .map_err(GraphFailure::ReqwestError)
    }

    async fn delete_calendar<ID: AsRef<str>>(&self, calendar_id: ID) -> GraphResult<()> {
        self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .delete_calendars()
            .send()
            .await?;
        Ok(())
    }

    async fn list_events_ids<ID: AsRef<str>>(&self, calendar_id: ID) -> GraphResult<Vec<String>> {
        Ok(self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .list_events()
            .select(&["id"])
            .top(100.to_string())
            .paging()
            .json::<serde_json::Value>()
            .await?
            .iter()
            .flat_map(|resp| resp.body())
            .flat_map(|body| match body["value"].as_array() {
                Some(arr) => arr.iter().map(|e| e["id"].as_str().map(|s| s.to_string())).collect(),
                None => vec![],
            })
            .flatten()
            .collect())
    }

    async fn list_events<ID: AsRef<str>>(&self, calendar_id: ID) -> GraphResult<EventsListResponse> {
        let stream = 
            self.client
                .lock()
                .await
                .me()
                .calendar(calendar_id)
                .calendar_views()
                .delta()
                .paging()
                .stream::<serde_json::Value>()?;

        Ok(self.extract_paged_delta_result(Box::new(stream)).await?.into())
    }

    async fn list_events_delta<ID: AsRef<str>, Token: AsRef<str>>(
        &self,
        calendar_id: ID,
        delta_token: Token,
    ) -> GraphResult<EventsListResponse> {
        let stream =
            self.client
            .lock()
            .await
            .me()
            .calendar_view(calendar_id)
            .append_query_pair("startDateTime", &self.min_sync_date.unwrap_or_else(Utc::now).to_rfc3339())
            .append_query_pair("endDateTime", "2100-01-01T00:00:00Z")
            .delta_token(delta_token)
            .get_calendar_view()
            .paging()
            .stream::<serde_json::Value>()?;

        Ok(self.extract_paged_delta_result(Box::new(stream)).await?.into())
    }

    async fn list_events_changed_since<ID: AsRef<str> + Send>(&self, calendar_id: ID, changed_since: DateTime<Utc>) -> GraphResult<Vec<Event>> {
        let stream = 
            self.client
                .lock()
                .await
                .me()
                .calendar(calendar_id)
                .events()
                .list_events()
                .filter(&[&format!("lastModifiedDateTime ge {}", changed_since.to_rfc3339())])
                .paging()
                .stream::<serde_json::Value>()?;

        Ok(self.extract_paged_delta_result(Box::new(stream)).await?.0)
    }

    async fn create_event<ID: AsRef<str>>(
        &self,
        calendar_id: ID,
        event: Event,
    ) -> GraphResult<Event> {
        self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .create_events(&event)
            .send()
            .await?
            .json()
            .await
            .map_err(GraphFailure::ReqwestError)
    }

    async fn update_event<ID: AsRef<str>>(
        &self,
        calendar_id: ID,
        event: Event,
    ) -> GraphResult<Event> {
        self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .update_events(&event.id, &event)
            .send()
            .await?
            .json()
            .await
            .map_err(GraphFailure::ReqwestError)
    }

    async fn delete_event<ID: AsRef<str>>(&self, calendar_id: ID, event_id: ID) -> GraphResult<()> {
        self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .delete_events(event_id)
            .send()
            .await?;
        Ok(())
    }
}

#[cfg(test)]
mod testing {
    use super::*;
    use http_test_server::TestServer;

    async fn resource(server: &TestServer) -> Resource {
        let configuration = GraphClientConfiguration::default()
            .access_token("access_token")
            .https_only(false);
        Resource::new_with_configuration(configuration)
            .use_test_endpoint(
                &Url::parse(&format!("http://127.0.0.1:{}/v1.0", server.port()))
                    .expect("Invalid URL"),
            )
            .await
    }

    #[tokio::test]
    async fn test_list_calendars() {
        let server = TestServer::new().expect("Failed to create test HTTP server");
        server.create_resource("/v1.0/me/calendars")
            .header("Content-Type", "application/json")
            .body(r#"{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('48d31887-5fad-4d73-a9f5-3c356e68a038')/calendars",
    "value": [
        {
            "id": "AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=",
            "name": "Calendar",
            "color": "auto",
            "hexColor": "",
            "isDefaultCalendar": true,
            "changeKey": "IiLKjG2I7E+Xv0+ys6MD0wAAAbhA9A==",
            "canShare": true,
            "canViewPrivateItems": true,
            "canEdit": true,
            "allowedOnlineMeetingProviders": [
                "teamsForBusiness"
            ],
            "defaultOnlineMeetingProvider": "teamsForBusiness",
            "isTallyingResponses": true,
            "isRemovable": false,
            "owner": {
                "name": "Megan Bowen",
                "address": "MeganB@M365x214355.onmicrosoft.com"
            }
        }
    ]
}"#);
        let resource = resource(&server).await;
        let calendars = resource.list_calendars().await.expect("Request failed");
        assert_eq!(calendars.len(), 1);
        let calendar = calendars.first().unwrap();
        assert_eq!(calendar.id, "AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=");
        assert_eq!(calendar.name, "Calendar");
        assert_eq!(calendar.change_key, "IiLKjG2I7E+Xv0+ys6MD0wAAAbhA9A==");
        assert!(calendar.hex_color.is_some());
        assert_eq!(calendar.hex_color.as_ref().unwrap(), "");
        assert!(calendar.can_edit)
    }

    #[tokio::test]
    async fn test_list_events() {
        let server = TestServer::new().expect("Failed to create test HTTP server");
        let port = server.port();
        server.create_resource("/v1.0/me/calendars/AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=/events")
            .header("Content-Type", "application/json")
            .body_fn(move |_| {
r#"{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('48d31887-5fad-4d73-a9f5-3c356e68a038')/events(subject,body,bodyPreview,organizer,attendees,start,end,location)",
    "value": [
        {
            "@odata.etag": "W/\"IiLKjG2I7E+Xv0+ys6MD0wAGQWZx+Q==\"",
            "id": "AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAENAAAiIsqMbYjsT5e-T7KzowPTAAAa_WKzAAA=",
            "subject": "New Product Regulations Touchpoint",
            "bodyPreview": "New Product Regulations Strategy Online Touchpoint Meeting\r\n\r\n\r\nYou're receiving this message because you're a member of the Engineering group. If you don't want to receive any messages or events from this group, stop following it in your inbox.\r\n\r\nView g",
            "body": {
                "contentType": "html",
                "content": "<html>\r\n<head>\r\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n</head>\r\n<body>\r\nNew Product Regulations Strategy Online Touchpoint Meeting\r\n<div id=\"a59ada49-a492-4f1d-ac57-74be3a4194fc\" style=\"display:inline-block\">\r\n<table cellspacing=\"0\" style=\"table-layout:fixed; width:50px; border:0 none black\">\r\n<tbody>\r\n<tr>\r\n<td style=\"height:18px; padding:0; border-width:0 0 1px 0; border-style:none none solid none; border-color:#EAEAEA\">\r\n&nbsp;</td>\r\n</tr>\r\n</tbody>\r\n</table>\r\n<table cellspacing=\"0\" style=\"table-layout:fixed; width:90%; line-height:17px; border:0 none black\">\r\n<tbody>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n<tr>\r\n<td style=\"padding:0; border:0 none black; color:#666666; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">\r\nYou're receiving this message because you're a member of the <a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=conversations\" style=\"color:#0072C6; text-decoration:none; font-size:12px; font-family:'Segoe UI Semibold','Segoe WP Semibold','Segoe UI','Segoe WP',sans-serif\">\r\nEngineering</a> group. If you don't want to receive any messages or events from this group,\r\n<a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=unsubscribe\" id=\"BD5134C6-8D33-4ABA-A0C4-08581FDF89DB\" style=\"color:#0072C6; text-decoration:none; font-size:12px; font-family:'Segoe UI Semibold','Segoe WP Semibold','Segoe UI','Segoe WP',sans-serif\">\r\nstop following it in your inbox</a>.</td>\r\n</tr>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n<tr>\r\n<td style=\"padding:0; border:0 none black; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">\r\n<span style=\"display:inline-block\"><a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=conversations\" style=\"color:#666666; text-decoration:none; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">View\r\n group conversations</a></span><span style=\"color:#C8C8C8\">&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;</span><span style=\"display:inline-block\"><a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=files\" style=\"color:#666666; text-decoration:none; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">View\r\n group files</a></span></td>\r\n</tr>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n</tbody>\r\n</table>\r\n</div>\r\n</body>\r\n</html>\r\n"
            },
            "start": {
                "dateTime": "2014-11-03T17:00:00.0000000",
                "timeZone": "UTC"
            },
            "end": {
                "dateTime": "2014-11-03T17:30:00.0000000",
                "timeZone": "UTC"
            },
            "location": {
                "displayName": "Conf Room Rainier",
                "locationType": "default",
                "uniqueId": "Conf Room Rainier",
                "uniqueIdType": "private"
            },
            "attendees": [
                {
                    "type": "required",
                    "status": {
                        "response": "none",
                        "time": "0001-01-01T00:00:00Z"
                    },
                    "emailAddress": {
                        "name": "Engineering",
                        "address": "engineering@M365x214355.onmicrosoft.com"
                    }
                },
                {
                    "type": "required",
                    "status": {
                        "response": "none",
                        "time": "0001-01-01T00:00:00Z"
                    },
                    "emailAddress": {
                        "name": "Irvin Sayers",
                        "address": "IrvinS@M365x214355.onmicrosoft.com"
                    }
                }
            ],
            "organizer": {
                "emailAddress": {
                    "name": "Engineering",
                    "address": "engineering@M365x214355.onmicrosoft.com"
                }
            }
        }
    ],
    "@odata.nextLink": "http://127.0.0.1:{port}/v1.0/me/calendars/AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=/events?%24top=10&%24skip=10"
}"#.replace("{port}", port.to_string().as_str())
            });

        server.create_resource("/v1.0/me/calendars/AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=/events")
            .query("%24top", "10")
            .query("%24skip", "10")
            .body(r#"{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('48d31887-5fad-4d73-a9f5-3c356e68a038')/events",
    "value": [
        {
            "@odata.etag": "W/\"IiLKjG2I7E+Xv0+ys6MD0wAGQWZx/A==\"",
            "id": "AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAENAAAiIsqMbYjsT5e-T7KzowPTAAAa_WKyAAA=",
            "createdDateTime": "2017-09-08T06:00:28.0922072Z",
            "lastModifiedDateTime": "2024-04-04T21:48:52.6141434Z",
            "changeKey": "IiLKjG2I7E+Xv0+ys6MD0wAGQWZx/A==",
            "categories": [],
            "transactionId": null,
            "originalStartTimeZone": "UTC",
            "originalEndTimeZone": "UTC",
            "iCalUId": "040000008200E00074C5B7101A82E008000000001DE8F4B16728D3010000000000000000100000005232C5046A593E4EB2CB35F84B01A566",
            "uid": "040000008200E00074C5B7101A82E008000000001DE8F4B16728D3010000000000000000100000005232C5046A593E4EB2CB35F84B01A566",
            "reminderMinutesBeforeStart": 15,
            "isReminderOn": true,
            "hasAttachments": false,
            "subject": "Latin American Product Manual Group",
            "bodyPreview": "Focus group to evaluate and update product manuals in South and Central America\r\n\r\n\r\nYou're receiving this message because you're a member of the Engineering group. If you don't want to receive any messages or events from this group, stop following it in ",
            "importance": "normal",
            "sensitivity": "normal",
            "isAllDay": false,
            "isCancelled": false,
            "isOrganizer": false,
            "responseRequested": true,
            "seriesMasterId": null,
            "showAs": "tentative",
            "type": "seriesMaster",
            "webLink": "https://outlook.office365.com/owa/?itemid=AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e%2FT7KzowPTAAAAAAENAAAiIsqMbYjsT5e%2FT7KzowPTAAAa%2BWKyAAA%3D&exvsurl=1&path=/calendar/item",
            "onlineMeetingUrl": null,
            "isOnlineMeeting": false,
            "onlineMeetingProvider": "unknown",
            "allowNewTimeProposals": true,
            "occurrenceId": null,
            "isDraft": false,
            "hideAttendees": false,
            "responseStatus": {
                "response": "notResponded",
                "time": "0001-01-01T00:00:00Z"
            },
            "body": {
                "contentType": "html",
                "content": "<html>\r\n<head>\r\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n</head>\r\n<body>\r\nFocus group to evaluate and update product manuals in South and Central America\r\n<div id=\"a59ada49-a492-4f1d-ac57-74be3a4194fc\" style=\"display:inline-block\">\r\n<table cellspacing=\"0\" style=\"table-layout:fixed; width:50px; border:0 none black\">\r\n<tbody>\r\n<tr>\r\n<td style=\"height:18px; padding:0; border-width:0 0 1px 0; border-style:none none solid none; border-color:#EAEAEA\">\r\n&nbsp;</td>\r\n</tr>\r\n</tbody>\r\n</table>\r\n<table cellspacing=\"0\" style=\"table-layout:fixed; width:90%; line-height:17px; border:0 none black\">\r\n<tbody>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n<tr>\r\n<td style=\"padding:0; border:0 none black; color:#666666; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">\r\nYou're receiving this message because you're a member of the <a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=conversations\" style=\"color:#0072C6; text-decoration:none; font-size:12px; font-family:'Segoe UI Semibold','Segoe WP Semibold','Segoe UI','Segoe WP',sans-serif\">\r\nEngineering</a> group. If you don't want to receive any messages or events from this group,\r\n<a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=unsubscribe\" id=\"BD5134C6-8D33-4ABA-A0C4-08581FDF89DB\" style=\"color:#0072C6; text-decoration:none; font-size:12px; font-family:'Segoe UI Semibold','Segoe WP Semibold','Segoe UI','Segoe WP',sans-serif\">\r\nstop following it in your inbox</a>.</td>\r\n</tr>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n<tr>\r\n<td style=\"padding:0; border:0 none black; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">\r\n<span style=\"display:inline-block\"><a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=conversations\" style=\"color:#666666; text-decoration:none; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">View\r\n group conversations</a></span><span style=\"color:#C8C8C8\">&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;</span><span style=\"display:inline-block\"><a href=\"https://outlook.office365.com/owa/engineering@M365x214355.onmicrosoft.com/groupsubscription.ashx?realm=M365x214355.onmicrosoft.com&amp;source=EscalatedMessage&amp;action=files\" style=\"color:#666666; text-decoration:none; font-size:12px; font-family:'Segoe UI','Segoe WP',sans-serif\">View\r\n group files</a></span></td>\r\n</tr>\r\n<tr>\r\n<td style=\"height:17px; padding:0; border:0 none black\">&nbsp;</td>\r\n</tr>\r\n</tbody>\r\n</table>\r\n</div>\r\n</body>\r\n</html>\r\n"
            },
            "start": {
                "dateTime": "2015-01-05T21:00:00.0000000",
                "timeZone": "UTC"
            },
            "end": {
                "dateTime": "2015-01-05T22:30:00.0000000",
                "timeZone": "UTC"
            },
            "location": {
                "displayName": "Mt. Adams",
                "locationType": "default",
                "uniqueId": "Mt. Adams",
                "uniqueIdType": "private"
            },
            "locations": [
                {
                    "displayName": "Mt. Adams",
                    "locationType": "default",
                    "uniqueId": "Mt. Adams",
                    "uniqueIdType": "private"
                }
            ],
            "recurrence": {
                "pattern": {
                    "type": "relativeMonthly",
                    "interval": 2,
                    "month": 0,
                    "dayOfMonth": 0,
                    "daysOfWeek": [
                        "monday"
                    ],
                    "firstDayOfWeek": "sunday",
                    "index": "first"
                },
                "range": {
                    "type": "noEnd",
                    "startDate": "2015-01-05",
                    "endDate": "0001-01-01",
                    "recurrenceTimeZone": "UTC",
                    "numberOfOccurrences": 0
                }
            },
            "attendees": [
                {
                    "type": "required",
                    "status": {
                        "response": "none",
                        "time": "0001-01-01T00:00:00Z"
                    },
                    "emailAddress": {
                        "name": "Engineering",
                        "address": "engineering@M365x214355.onmicrosoft.com"
                    }
                },
                {
                    "type": "required",
                    "status": {
                        "response": "none",
                        "time": "0001-01-01T00:00:00Z"
                    },
                    "emailAddress": {
                        "name": "Ben Walters",
                        "address": "BenW@M365x214355.onmicrosoft.com"
                    }
                }
            ],
            "organizer": {
                "emailAddress": {
                    "name": "Engineering",
                    "address": "engineering@M365x214355.onmicrosoft.com"
                }
            },
            "onlineMeeting": null,
            "calendar@odata.associationLink": "https://graph.microsoft.com/v1.0/users('48d31887-5fad-4d73-a9f5-3c356e68a038')/calendars('AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=')/$ref",
            "calendar@odata.navigationLink": "https://graph.microsoft.com/v1.0/users('48d31887-5fad-4d73-a9f5-3c356e68a038')/calendars('AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=')"
        }
    ]
}"#);

        let resource = resource(&server).await;
        let resp= resource.list_events("AAMkAGVmMDEzMTM4LTZmYWUtNDdkNC1hMDZiLTU1OGY5OTZhYmY4OABGAAAAAAAiQ8W967B7TKBjgx9rVEURBwAiIsqMbYjsT5e-T7KzowPTAAAAAAEGAAAiIsqMbYjsT5e-T7KzowPTAAABuC35AAA=")
            .await
            .expect("Failed to request events");
        assert_eq!(resp.events.len(), 2);
    }
}
