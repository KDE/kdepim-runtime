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
    use crate::{calendar::CalendarColor, testing::GraphExplorerProxy};

    async fn resource(proxy: &GraphExplorerProxy) -> Resource {
        let configuration = GraphClientConfiguration::default()
            .access_token("access_token")
            .https_only(false);
        Resource::new_with_configuration(configuration)
            .use_test_endpoint(
                &Url::parse(&format!("http://127.0.0.1:{}/", proxy.port))
                    .expect("Invalid URL"),
            )
            .await
    }

    #[tokio::test]
    async fn test_list_calendars() {
        let proxy = GraphExplorerProxy::run();
        let resource = resource(&proxy).await;

        let calendars = resource.list_calendars().await.expect("Request failed");

        assert_eq!(calendars.len(), 1);
        let calendar = calendars.first().unwrap();
        assert_eq!(calendar.name, "Calendar");
        assert!(calendar.can_edit);
        assert_eq!(calendar.color, CalendarColor::Auto);
        assert!(calendar.hex_color.is_none());
    }

    #[tokio::test]
    async fn test_list_events() {
        let proxy = GraphExplorerProxy::run();
        let resource = resource(&proxy).await;

        let calendars = resource.list_calendars().await.expect("Calendar request failed");
        let calendar_id = calendars.iter().find(|c| c.is_default_calendar).expect("No default calendar found").id.as_str();

        let resp= resource.list_events(calendar_id)
            .await
            .expect("Failed to request events");
        println!("{:?}", resp.events.len());
        println!("{:?}", resp.events);
    }
}
