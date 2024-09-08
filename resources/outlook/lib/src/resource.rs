use graph_rs_sdk::GraphClient;
use anyhow::Error;
use serde::Deserialize;

use crate::calendar::Calendar;
use crate::resource_state::{Collection, Item};

pub struct Resource {
    client: GraphClient,
}

#[derive(Debug, Clone, Deserialize)]
pub struct GraphResponse<T> {
    values: Vec<T> 
}

impl Default for Resource {
    fn default() -> Self {
        Self {
            client: GraphClient::new("1234")
        }
    }
}

impl Resource {
    pub async fn sync_collection(
        &self,
        _collection: Collection,
    ) -> Result<CollectionSyncResult, Error> {
        todo!()
    }

    pub async fn sync_collection_tree(&self) -> Result<Vec<Collection>, Error> {
        Ok(self.client
            .me()
            .calendars()
            .list_calendars()
            .paging()
            .json::<GraphResponse<Calendar>>()
            .await?
            .iter()
            .flat_map(|resp| resp.body())
            .flat_map(|resp| resp.values.clone())
            .map(|calendar| Collection {
                id: -1,
                name: calendar.name,
                remote_id: calendar.id,
                remote_revision: calendar.change_key
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

pub enum CollectionSyncResult {
    Incremental {
        added: Vec<Item>,
        removed: Vec<Item>,
    },
    Full {
        items: Vec<Item>,
    },
}

