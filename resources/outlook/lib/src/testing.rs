use reqwest::{blocking::Client, Method};
use rouille::{Request, Response, ResponseBody, Server};
use std::{
    borrow::Cow,
    io::Read,
    sync::mpsc::Sender,
    thread::JoinHandle,
};
use url::Url;

pub struct GraphExplorerProxy {
    pub port: u16,
    handle: Option<JoinHandle<()>>,
    stop: Sender<()>,
}

impl GraphExplorerProxy {
    pub fn run() -> Self {
        let server = Server::new("127.0.0.1:0", move |request| {
            Self::handle_request(request)
        })
        .expect("Failed to start server");
        let port = server.server_addr().port();
        let (handle, stop) = server.stoppable();

        Self {
            port,
            handle: Some(handle),
            stop,
        }
    }

    fn handle_request(request: &Request) -> Response {
        let url = Url::parse_with_params(
            "https://graph.office.net/en-us/graph/api/proxy",
            [(
                "url",
                format!("https://graph.microsoft.com/v1.0{}", request.raw_url()).as_str(),
            )],
        )
        .expect("Failed to parse URL");
        println!("{:?}", request.raw_url());

        let mut proxy_resp = tokio::task::block_in_place(move || {
            // Constructing a new client for each request is not ideal, but it's the easiest way
            // right now. And after all, this is just a test server...
            let client = Client::new();
            let mut proxy_req = client
                .request(
                    match request.method() {
                        "GET" => Method::GET,
                        "POST" => Method::POST,
                        "PUT" => Method::PUT,
                        "DELETE" => Method::DELETE,
                        _ => panic!("Unsupported method"),
                    },
                    url,
                )
                .bearer_auth("{token:https://graph.microsoft.com/}");
            if let Some(body) = request.data() {
                proxy_req = proxy_req.body(
                    body.bytes()
                        .take_while(Result::is_ok)
                        .map(Result::unwrap)
                        .collect::<Vec<u8>>(),
                );
            }

            proxy_req.send().expect("Failed to send proxy request")
        });

        Response {
            status_code: proxy_resp.status().as_u16(),
            headers: proxy_resp
                .headers()
                .iter()
                .map(|(name, value)| {
                    (
                        Cow::Owned(name.as_str().into()),
                        Cow::Owned(value.to_str().unwrap().into()),
                    )
                })
                .collect(),
            data: {
                let mut data = Vec::new();
                proxy_resp
                    .read_to_end(&mut data)
                    .expect("Failed to read response body");
                //println!("{:?}", String::from_utf8_lossy(&data));
                ResponseBody::from_data(data)
            },
            upgrade: None,
        }
    }
}

impl Drop for GraphExplorerProxy {
    fn drop(&mut self) {
        self.stop.send(()).unwrap();
        self.handle.take().unwrap().join().unwrap();
    }
}

#[cfg(test)]
mod tests {
    use serde_json::Value;

    use super::*;

    #[tokio::test]
    async fn test_proxy() {
        let proxy = GraphExplorerProxy::run();
        let resp = reqwest::get(&format!("http://localhost:{}/me", proxy.port))
            .await
            .expect("Failed to make request");

        let json: Value = resp.json().await.expect("Failed to parse JSON");
        assert_eq!(json["displayName"], "Megan Bowen");
        assert_eq!(json["mail"], "MeganB@M365x214355.onmicrosoft.com");
    }
}
