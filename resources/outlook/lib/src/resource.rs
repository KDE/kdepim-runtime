use std::collections::HashMap;

use anyhow::Error;
use futures::{future, StreamExt, TryStreamExt};

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

impl Resource {
    async fn sync_collection_full(
        &self,
        collection: Collection,
    ) -> Result<CollectionSyncResult, Error> {
        let events = HashMap::<String, EventWithExceptions>::new();

        self
            .client
            .list_calendar_events(collection.remote_id)
            .await?
            .map(|event| match event {
                Ok(event) => Ok(EventWithExceptions {
                    master: event,
                    ..Default::default()
                }),
                Err(err) => Err(err),
            })
            .try_buffer_unordered(5, |event| async {
                let exceptions = match self.client.list_event_exceptions(&collection.remote_id, &event.master.id).await {
                    Ok(stream) => stream,
                    Err(err) => return future::ready(Err(err))
                }.try_collect().await;

                match exceptions {
                    Ok(exceptions) => {
                        event.exceptions = exceptions;
                        future::ready(Ok(event))
                    }
                    Err(err) => future::ready(Err(err))
                }
            })
            .await;

        self
            .client
            .list_events_exceptions(collection.remote_id)
            .await?
            .try_for_each(|event| match event {
                Some(EventType::SeriesMaster) => {
                    // TODO: validate that the master event already exists in the events map

                }
            })


        Ok(CollectionSyncResult::Full {
            items: result.events.iter().map(|event| Item {
                id: -1,
                remote_revision: event.change_key.as_ref().unwrap_or(&"".to_string()).clone(),
                remote_id: event.id.clone(),
                mime_type: "text/calendar".to_string(),
                payload: event.into()
            }).collect(),
            collection: Collection {
                remote_revision: result.delta_token.unwrap_or("".to_string()).to_string(),
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
