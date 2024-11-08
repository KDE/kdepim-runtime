use std::collections::HashMap;
use std::future::Future;

use anyhow::Error;
use futures::{future, FutureExt, StreamExt, TryFuture, TryFutureExt, TryStreamExt};
use graph_rs_sdk::GraphResult;

use crate::calendar::{CalendarAPI, Event, EventType};
use crate::client::Client;
use crate::resource_state::{Collection, Item};

pub struct Resource {
    client: Client,
}

impl Resource {
    pub fn new(access_token: String, _refresh_token: String) -> Self {
        Self {
            client: Client::new(access_token),
        }
    }

    pub async fn sync_collection(
        &self,
        collection: Collection,
    ) -> Result<CollectionSyncResult, Error> {
        if collection.remote_revision.is_empty() {
            self.sync_collection_full(collection).await
        } else {
            self.sync_collection_incremental(collection).await
        }
    }

    pub async fn sync_collection_tree(&self) -> Result<Vec<Collection>, Error> {
        Ok(self
            .client
            .list_calendars()
            .await?
            .iter()
            .map(|calendar| Collection {
                id: -1,
                name: calendar.name.clone(),
                remote_id: calendar.id.clone(),
                remote_revision: calendar.change_key.clone(),
            })
            .collect())
    }

    pub async fn add_collection(&self, _collection: Collection) -> Result<Collection, Error> {
        todo!()
    }

    pub async fn change_collection(&self, _collection: Collection) -> Result<Collection, Error> {
        todo!()
    }

    pub async fn remove_collection(&self) -> Result<(), Error> {
        todo!()
    }

    pub async fn add_item(&self, _item: Item) -> Result<Item, Error> {
        todo!()
    }

    pub async fn change_item(&self, _item: Item) -> Result<Item, Error> {
        todo!()
    }

    pub async fn remove_item(&self) -> Result<(), Error> {
        todo!()
    }
}

#[derive(Debug, Default)]
struct EventWithExceptions {
    master: Event,
    exceptions: Vec<Event>,
}

async fn fetch_exceptions_for_event<'a>(client: &'a Client, calendar_id: &'a str, event_id: String) -> GraphResult<Vec<Event>> {
    client
        .list_event_exceptions(calendar_id, &event_id)
        .await?
        .try_collect()
        .await
}

impl Resource {
    async fn sync_collection_full(
        &self,
        collection: Collection,
    ) -> Result<CollectionSyncResult, Error> {
        Ok(CollectionSyncResult::Full {
            items: {
                let mut events = Vec::<EventWithExceptions>::new();

                // Fetch events and their exceptions - the stream will short-circuit on any error
                // Unfortunatelly we have to materialize the interminnent results
                self
                .client
                .list_calendar_events(collection.remote_id)
                .await?
                .try_for_each_concurrent(5, |event| async {
                    events.push(
                        fetch_exceptions_for_event(&self.client, &collection.remote_id, event.id.clone())
                            .map_ok(|exceptions| EventWithExceptions {
                                master: event,
                                exceptions,
                            }).await?
                    );
                    Ok(())
                }).await?;

                events
                    .into_iter()
                    .map(|event| Item {
                        id: -1,
                        remote_revision: event.master.change_key.as_ref().unwrap_or(&"".to_string()).clone(),
                        remote_id: event.master.id.clone(),
                        mime_type: "text/calendar".to_string(),
                        payload: event.into(),
                    })
                    .collect()
            },
            collection: Collection {
                //remote_revision: result.delta_token.unwrap_or("".to_string()).to_string(),
                ..collection
            }
        })
    }

    async fn sync_collection_incremental(
        &self,
        _collection: Collection,
    ) -> Result<CollectionSyncResult, Error> {
        todo!()
    }
}

pub enum CollectionSyncResult {
    Incremental {
        added: Vec<Item>,
        removed: Vec<Item>,
        collection: Collection,
    },
    Full {
        items: Vec<Item>,
        collection: Collection,
    },
}
