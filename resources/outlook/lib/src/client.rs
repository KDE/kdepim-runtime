use std::{future::Future, pin::Pin};

use chrono::{DateTime, Utc};
use futures::{
    stream::{self, Iter, StreamExt},
    Stream,
};
use graph_http::{api_impl::PagingResult, traits::ODataDeltaLink};
use graph_rs_sdk::{
    error::ErrorMessage, http::HttpResponseExt, GraphClient, GraphClientConfiguration,
    GraphFailure, GraphResult, ODataQuery,
};
use http::Response;
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use serde_json::Value;
use tokio::sync::Mutex;
use url::Url;

use crate::calendar::{Calendar, CalendarAPI, Event, EventsListResponse};
pub struct Client {
    client: Mutex<GraphClient>,
    min_sync_date: Option<DateTime<Utc>>,
}

impl Client {
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
#[serde(untagged)]
pub enum GraphResponse<T> {
    Ok { value: T },
    Error(ErrorMessage),
}

impl<T> Into<GraphResult<T>> for GraphResponse<T> {
    fn into(self) -> GraphResult<T> {
        match self {
            GraphResponse::Ok { value } => Ok(value),
            GraphResponse::Error(err) => Err(GraphFailure::ErrorMessage(err)),
        }
    }
}

impl Client {
    async fn extract_paged_delta_result<T>(
        &self,
        mut stream: Box<
            impl Stream<Item = GraphResult<Response<Result<Value, ErrorMessage>>>> + Unpin,
        >,
    ) -> GraphResult<(Vec<T>, Option<String>)>
    where
        T: DeserializeOwned,
    {
        let mut events = Vec::<T>::new();
        let mut delta_token: Option<String> = None;
        while let Some(result) = stream.next().await {
            let response = result?;
            let body = response.into_body()?;

            if let Some(url) = body.odata_delta_link() {
                if let Some((_, token)) = Url::parse(&url)?
                    .query_pairs()
                    .find(|(key, _)| key == "$deltatoken")
                {
                    delta_token = Some(token.into_owned());
                }
            }

            let results: GraphResponse<Vec<T>> =
                serde_json::from_value(body.clone()).map_err(|e| {
                    println!("Failed to decode response: {:?}", body);
                    e
                })?;

            match results {
                GraphResponse::Error(err) => return Err(GraphFailure::ErrorMessage(err)),
                GraphResponse::Ok { value } => events.extend(value),
            }
        }

        // TODO: Return a stream of events instead so we can further process it
        Ok((events, delta_token))
    }
}

/// Takes a single PagingResult and converts it into a stream of elements contained
/// on the single page.
/// If fetching the page was an error, the page itself contained an error instead of the elements,
/// or we are unable to deserialize the page body, the stream will contain a single element with
/// the error.
fn paging_result_into_stream<T: Clone + 'static>(result: PagingResult<GraphResponse<Vec<T>>>) -> Iter<std::vec::IntoIter<GraphResult<T>>> {
    stream::iter(
        match result {
            Ok(response) => match response.body().clone() {
                Ok(GraphResponse::Ok { value }) => value.into_iter().map(Ok).collect(),
                Ok(GraphResponse::Error(err)) => vec![Err(GraphFailure::ErrorMessage(err))],
                Err(err) => vec![Err(GraphFailure::ErrorMessage(err))],
            },
            Err(err) => vec![Err(err)],
        }
    )
}

impl CalendarAPI for Client {
    async fn list_calendars(&self) -> GraphResult<Vec<Calendar>> {
        self.client
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
            .into()
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

    async fn list_events_instances<ID: AsRef<str>>(
        &self,
        calendar_id: ID,
    ) -> GraphResult<Pin<Box<dyn Stream<Item = GraphResult<Event>>>>> {
        Ok(
            self.client
                .lock()
                .await
                .me()
                .calendar(calendar_id)
                .calendar_views()
                .append_query_pair(
                    "startDateTime",
                    &self.min_sync_date.unwrap_or_else(Utc::now).to_rfc3339(),
                )
                .append_query_pair("endDateTime", "2100-01-01T00:00:00Z")
                .delta()
                .paging()
                .stream::<GraphResponse<Vec<Event>>>()? // create a stream of individual pages
                .flat_map(paging_result_into_stream) // flat_map it into a stream of individual events
                .boxed()
        )
    }

    async fn list_calendar_events<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
    ) -> GraphResult<Pin<Box<dyn Stream<Item = GraphResult<Event>>>>> {
        Ok(self.client
                .lock()
                .await
                .me()
                .calendar(calendar_id)
                .events()
                .list_events()
                .paging()
                .stream::<GraphResponse<Vec<Event>>>()? // create a stream of individual pages
                .flat_map(paging_result_into_stream) // flat_map it into a stream of individual events
                .boxed()
        )
    }

    async fn list_event_exceptions<ID: AsRef<str> + Send>(
        &self,
        calendar_id: ID,
        event_id: ID,
    ) -> GraphResult<Pin<Box<dyn Stream<Item = GraphResult<Event>>>>> {
        Ok(self.client
            .lock()
            .await
            .me()
            .calendar(calendar_id)
            .event(event_id)
            .instances()
            .filter(&vec!["type eq 'exception'"])
            .list_instances()
            .paging()
            .stream::<GraphResponse<Vec<Event>>>()?
            .flat_map(paging_result_into_stream)
            .boxed()
        )
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

    async fn resource(proxy: &GraphExplorerProxy) -> Client {
        let configuration = GraphClientConfiguration::default()
            .access_token("access_token")
            .https_only(false);
        Client::new_with_configuration(configuration)
            .use_test_endpoint(
                &Url::parse(&format!("http://127.0.0.1:{}/", proxy.port)).expect("Invalid URL"),
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
    async fn test_list_events_since() {
        let proxy = GraphExplorerProxy::run();
        let resource = resource(&proxy).await;

        let calendars = resource
            .list_calendars()
            .await
            .expect("Calendar request failed");
        let calendar_id = calendars
            .iter()
            .find(|c| c.is_default_calendar)
            .expect("No default calendar found")
            .id
            .as_str();

        let resp = resource
            .list_events_instances(calendar_id)
            .await
            .expect("Failed to request events");
        println!("{:?}", resp.events.len());
        println!("{:?}", resp.events);
    }
}
