use anyhow::Error;
use cxx_qt::Threading;
use cxx_qt_lib::QString;
use std::{
    fmt::{self, Debug, Formatter},
    pin::Pin,
    sync::Mutex,
    thread::{self, JoinHandle},
};
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};

use crate::resource::{CollectionSyncResult, Resource};
use crate::entities::{Item, Collection};

#[cxx_qt::bridge]
pub mod resource_state {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        /// An alias to the QString type
        type QString = cxx_qt_lib::QString;
    }

    extern "Rust" {
        type Item;
        type Collection;

    }

    unsafe extern "RustQt" {
        #[qobject]
        type ResourceState = super::ResourceStateRust;

        #[cxx_name = "retrieveItems"]
        fn retrieve_items(self: Pin<&mut ResourceState>, collection: &Box<Collection>);
        #[cxx_name = "retrieveCollections"]
        fn retrieve_collections(self: Pin<&mut ResourceState>);

        #[cxx_name = "itemAdded"]
        fn item_added(self: Pin<&mut ResourceState>, item: &Box<Item>);
        #[cxx_name = "itemChanged"]
        fn item_changed(self: Pin<&mut ResourceState>, item: &Box<Item>);
        #[cxx_name = "itemRemoved"]
        fn item_removed(self: Pin<&mut ResourceState>);

        #[cxx_name = "collectionAdded"]
        fn collection_added(self: Pin<&mut ResourceState>, collection: &Box<Collection>);
        #[cxx_name = "collectionChanged"]
        fn collection_changed(self: Pin<&mut ResourceState>, collection: &Box<Collection>);
        #[cxx_name = "collectionRemoved"]
        fn collection_removed(self: Pin<&mut ResourceState>);

        #[qsignal]
        #[cxx_name = "changeProcessed"]
        fn change_processed(self: Pin<&mut ResourceState>);
        #[qsignal]
        #[cxx_name = "itemsRetrieved"]
        fn items_retrieved(self: Pin<&mut ResourceState>, items: &Vec<Item>);
        #[qsignal]
        #[cxx_name = "itemsRetrievedIncremental"]
        fn items_retrieved_incremental(
            self: Pin<&mut ResourceState>,
            added_items: Vec<Item>,
            removed_items: Vec<Item>,
        );
        #[qsignal]
        #[cxx_name = "collectionsRetrieved"]
        fn collections_retrieved(self: Pin<&mut ResourceState>, collections: Vec<Collection>);
        #[qsignal]
        #[cxx_name = "taskDone"]
        fn task_done(self: Pin<&mut ResourceState>);
        #[qsignal]
        #[cxx_name = "taskFailed"]
        fn task_failed(self: Pin<&mut ResourceState>, error: QString);
    }

    impl cxx_qt::Constructor<(QString, QString), NewArguments = (QString, QString)> for ResourceState {}

    impl cxx_qt::Threading for ResourceState {}
}


enum ResourceTask {
    SyncCollectionTree {
        result_handler: Box<dyn FnOnce(ResourceTaskResult) + Send>,
    },
    SyncCollectionFull {
        collection: Collection,
        result_handler: Box<dyn FnOnce(ResourceTaskResult) + Send>,
    },
    Quit,
}

impl Debug for ResourceTask {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        match self {
            ResourceTask::SyncCollectionTree { .. } => write!(f, "SyncCollectionTree"),
            ResourceTask::SyncCollection { collection, .. } => {
                write!(f, "SyncCollection({:?})", collection)
            }
            ResourceTask::Quit => write!(f, "Quit"),
        }
    }
}

enum ResourceTaskResult {
    ItemsRetreved {
        items: Vec<Item>,
    },
    ItemsRetrievedIncremental {
        added: Vec<Item>,
        removed: Vec<Item>,
    },
    CollectionsRetrieved {
        collections: Vec<Collection>,
    },
    TaskDone {
        result: Result<(), Error>,
    },
}

impl Debug for ResourceTaskResult {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        match self {
            ResourceTaskResult::ItemsRetreved { .. } => write!(f, "ItemsRetreved"),
            ResourceTaskResult::ItemsRetrievedIncremental { .. } => {
                write!(f, "ItemsRetrievedIncremental")
            }
            ResourceTaskResult::CollectionsRetrieved { .. } => write!(f, "CollectionsRetrieved"),
            ResourceTaskResult::TaskDone { .. } => write!(f, "TaskDone"),
        }
    }
}

pub struct ResourceStateRust {
    channel: UnboundedSender<ResourceTask>,
    worker_thread: Option<JoinHandle<()>>,
}

impl cxx_qt::Constructor<(QString, QString)> for resource_state::ResourceState {
    type BaseArguments = ();
    type InitializeArguments = ();
    type NewArguments = (QString, QString);

    fn route_arguments(
        args: (QString, QString),
    ) -> (
        Self::NewArguments,
        Self::BaseArguments,
        Self::InitializeArguments,
    ) {
        (args, (), ())
    }

    fn new((access_token, refresh_token): (QString, QString)) -> ResourceStateRust {
        let (tx, rx) = unbounded_channel();
        ResourceStateRust {
            channel: tx,
            worker_thread: Some(thread::spawn(move || {
                let runtime = tokio::runtime::Builder::new_current_thread()
                    .enable_all()
                    .build()
                    .expect("Failed to create Tokio runtime");
                runtime.block_on(resource_loop(
                    rx,
                    (&access_token).into(),
                    (&refresh_token).into(),
                ));
            })),
        }
    }
}

impl Drop for ResourceStateRust {
    fn drop(&mut self) {
        if let Err(err) = self.channel.send(ResourceTask::Quit) {
            eprintln!("Failed to send task Quit: {:?}", err);
        }
        assert!(self.worker_thread.is_some());
        if let Err(err) = self.worker_thread.take().unwrap().join() {
            eprintln!("Failed to join worker thread: {:?}", err);
        }
    }
}

unsafe impl Sync for ResourceStateRust {}
unsafe impl Send for ResourceStateRust {}

impl resource_state::ResourceState {
    pub fn retrieve_items(self: Pin<&mut Self>, collection: Collection) {
        let qt_thread = Mutex::new(self.qt_thread());
        if let Err(err) = self.channel.send(ResourceTask::SyncCollection {
            collection,
            result_handler: Box::new(move |result| {
                if let Err(err) = qt_thread.lock().unwrap().queue(move |this| match result {
                    ResourceTaskResult::ItemsRetreved { items } => {
                        this.items_retrieved(items);
                    }
                    ResourceTaskResult::ItemsRetrievedIncremental { added, removed } => {
                        this.items_retrieved_incremental(added, removed);
                    }
                    ResourceTaskResult::TaskDone { result } => match result {
                        Ok(()) => this.change_processed(),
                        Err(error) => this.task_failed((&error.to_string()).into()),
                    },
                    _ => panic!("Unexpected result {:?} for task SyncCollection", result),
                }) {
                    eprintln!("Failed to send task SyncCollection: {:?}", err);
                }
            }),
        }) {
            eprintln!("Failed to send task SyncCollection: {:?}", err);
        }
    }

    pub fn retrieve_collections(self: Pin<&mut Self>) {
        let qt_thread = Mutex::new(self.qt_thread());
        if let Err(err) = self.channel.send(ResourceTask::SyncCollectionTree {
            result_handler: Box::new(move |result| {
                if let Err(err) = qt_thread.lock().unwrap().queue(move |this| match result {
                    ResourceTaskResult::CollectionsRetrieved { collections } => {
                        this.collections_retrieved(collections);
                    }
                    ResourceTaskResult::TaskDone { result } => match result {
                        Ok(()) => this.change_processed(),
                        Err(error) => this.task_failed((&error.to_string()).into()),
                    },
                    _ => panic!("Unexpected result {:?} for task SyncCollectionTree", result),
                }) {
                    eprintln!("Failed to send task SyncCollectionTree: {:?}", err);
                }
            }),
        }) {
            eprintln!("Failed to send task SyncCollectionTree: {:?}", err);
        }
    }

    pub fn collection_added(self: Pin<&mut Self>, _collection: Collection) {
        todo!();
    }

    pub fn collection_changed(self: Pin<&mut Self>, _collection: Collection) {
        todo!();
    }

    pub fn collection_removed(self: Pin<&mut Self>) {
        todo!();
    }

    pub fn item_added(self: Pin<&mut Self>, _item: Item) {
        todo!();
    }

    pub fn item_changed(self: Pin<&mut Self>, _item: Item) {
        todo!();
    }

    pub fn item_removed(self: Pin<&mut Self>) {
        todo!();
    }
}

async fn resource_loop(
    mut channel: UnboundedReceiver<ResourceTask>,
    access_token: String,
    refresh_token: String,
) {
    let resource = Resource::new(access_token, refresh_token);

    loop {
        match channel.recv().await {
            None => {
                return;
            }
            Some(ResourceTask::SyncCollectionTree { result_handler }) => {
                match resource.sync_collection_tree().await {
                    Ok(collections) => {
                        result_handler(ResourceTaskResult::CollectionsRetrieved { collections });
                    }
                    Err(error) => {
                        result_handler(ResourceTaskResult::TaskDone { result: Err(error) });
                    }
                }
            }
            Some(ResourceTask::SyncCollection {
                collection,
                result_handler,
            }) => match resource.sync_collection(collection).await {
                Ok(result) => match result {
                    CollectionSyncResult::Incremental { added, removed } => {
                        result_handler(ResourceTaskResult::ItemsRetrievedIncremental {
                            added,
                            removed,
                        });
                    }
                    CollectionSyncResult::Full { items } => {
                        result_handler(ResourceTaskResult::ItemsRetreved { items });
                    }
                },
                Err(error) => {
                    result_handler(ResourceTaskResult::TaskDone { result: Err(error) });
                }
            },
            Some(ResourceTask::Quit) => {
                return;
            }
        }
    }
}
