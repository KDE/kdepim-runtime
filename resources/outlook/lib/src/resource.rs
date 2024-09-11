use anyhow::Error;
use graph_rs_sdk::GraphClient;
use itertools::Itertools;
use serde::Deserialize;

use crate::calendar::Calendar;
use crate::resource_state::{Collection, Item};

pub struct Resource {
    client: GraphClient,
}

#[derive(Debug, Clone, Deserialize)]
pub struct GraphError {
    #[allow(dead_code)]
    code: String,
    message: String,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(untagged)]
pub enum GraphResponse<T> {
    Error { error: GraphError },
    Value { value: Vec<T> },
}

impl Resource {
    pub fn new(access_token: String, _refresh_token: String) -> Self {
        Self {
            client: GraphClient::new(access_token),
        }
    }

    pub async fn sync_collection(
        &self,
        collection: Collection
    ) -> Result<CollectionSyncResult, Error> {
        if collection.remote_revision.is_empty() {
            self.sync_collection_full(collection)
        } else {
            self.sync_collection_incremental(collection)
        }
    }
    
    pub async fn sync_collection_tree(&self) -> Result<Vec<Collection>, Error> {
        let response = self
            .client
            .me()
            .calendars()
            .list_calendars()
            .paging()
            .json::<GraphResponse<Calendar>>()
            .await?;

        response
            .iter()
            .flat_map(|resp| resp.body())
            .map(|body| match body {
                GraphResponse::Error { error } => Err(Error::msg(format!(
                    "Graph error: {} ({})",
                    error.message, error.code
                ))),
                GraphResponse::Value { value } => Ok(value.iter().map(|calendar| Collection {
                    id: -1,
                    name: calendar.name.clone(),
                    remote_id: calendar.id.clone(),
                    remote_revision: calendar.change_key.clone(),
                })),
            })
            .flatten_ok()
            .collect()
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

impl Resource {
    fn sync_collection_full(&self, collection: Collection) -> Result<CollectionSyncResult, Error>
    {
        Ok(
            CollectionSynResult::Full {
                items: self.client.me().calendar(collection.remote_id).list_events().paging().json::<GraphResponse<Item>>().await?.iter().flat_map(|resp| resp.body()).map(|body| match body {
                    GraphResponse::Error { error } => Err(Error::msg(format!("Graph error: {} ({})", error.message, error.code))),
                    GraphResponse::Value { value } => Ok(value.iter().map(|event| Item {
                        id: -1,
                        remote_id: event.id.clone(),
                        remote_revision: event.change_key.clone(),
                        payload: Box::new(event)
                    }))
                })
                .flatten_ok()
                .collect()?,
                collection
            }
        )
    }

    fn sync_collection_incremental(&self, collection: Collection) -> Result<CollectionSyncResult>
    {

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
