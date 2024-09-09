mod auth;

use auth::get_access_token;
use text_io::read;

use libgraph::resource::Resource;


#[tokio::main]
async fn main() {
    let (access_token, refresh_token) = get_access_token().await.expect("Failed to obtain access_token: {}");
    println!("Received access token: {access_token}");
    println!("Refresh token: {refresh_token}");

    let resource = Resource::new(access_token, refresh_token);
    let calendars = resource.sync_collection_tree().await.expect("Failed to fetch calendars: {}");
    calendars.iter().enumerate().for_each(|(idx, calendar)| {
        println!("{}) {}", idx + 1, calendar.name);
    });

    /*
    print!("Select a calendar (1-{}): ", calendars.len());
    let idx: u16 = read!("{}\n");

    let calendar = &calendars[idx as usize - 1];
    let events = resource.sync_collection(*calendar).await.expect("Failed to fetch events: {}");
    events.iter().for_each(|event| {
        println!("{}: {}", event.start.date_time, event.subject);
    })
    */
}
