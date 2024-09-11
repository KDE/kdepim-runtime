pub trait Payload {}
pub type DynPayload = Box<dyn Payload>;


#[derive(Debug)]
pub struct Collection {
    id: i64,
    name: String,
    remote_revision: String,
    remote_id: String,
}

#[derive(Debug)]
pub struct Item {
    id: i64,
    remote_revision: String,
    remote_id: String,
    payload: Box<DynPayload>,
}

