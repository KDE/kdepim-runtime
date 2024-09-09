use std::collections::HashMap;

use anyhow::Result;
use axum::debug_handler;
use axum::http::Response;
use axum::{
    extract::{Query, State},
    response::IntoResponse,
    routing::get,
};
use graph_oauth::AuthorizationCodeCredential;
use reqwest::{self, Method};
use serde::Deserialize;
use tokio::sync::mpsc::UnboundedSender;
use tokio::sync::oneshot;
use url::Url;

static CLIENT_ID: &str = "29854e75-de16-4284-9c28-cbabb2344bf9";
static TENANT: &str = "common";
static SCOPE: [&str; 2] = ["Calendars.ReadWrite", "offline_access"];

enum Authentication {
    Port(u16),
    AccessCode(String),
    Error(String),
}

#[derive(Debug, Clone)]
struct ServerState {
    tx: UnboundedSender<Authentication>,
}

#[derive(Debug, Clone, Deserialize)]
struct AuthQuery {
    code: Option<String>,
    #[allow(dead_code)]
    error: Option<String>,
    error_description: Option<String>,
}

fn get_authorization_code_url(port: u16) -> Result<Url> {
    Ok(
        AuthorizationCodeCredential::authorization_url_builder(CLIENT_ID)
            .with_tenant(TENANT)
            .with_redirect_uri(Url::parse(&format!("http://localhost:{port}/redirect"))?)
            .with_scope(SCOPE)
            .with_login_hint("me@dvratil.cz")
            .build()
            .url()?,
    )
}

#[derive(Debug, Clone, Deserialize)]
struct TokenResponse {
    access_token: Option<String>,
    refresh_token: Option<String>,
    #[allow(dead_code)]
    error: Option<String>,
    error_description: Option<String>,
}

async fn access_token_from_authorization_code(port: u16, code: String) -> Result<(String, String)> {
    // FIXME: We could use AuthorizationCodeCredential for it, but due to a bug it insists
    // on sending client_secret, even though it's not permitted for desktop apps
    // See https://github.com/sreeise/graph-rs-sdk/pull/493
    let client = reqwest::Client::new();
    let mut params = HashMap::<String, String>::new();
    params.insert("client_id".to_string(), CLIENT_ID.to_string());
    params.insert("scope".to_string(), SCOPE.join(" "));
    params.insert("code".to_string(), code);
    params.insert("redirect_uri".to_string(), format!("http://localhost:{port}/redirect"));
    params.insert("grant_type".to_string(), "authorization_code".to_string());
    let response = client.request(Method::POST, "https://login.microsoftonline.com/common/oauth2/v2.0/token")
        .form(&params)
        .send()
        .await?
        .json::<TokenResponse>()
        .await?;
    if let Some(access_token) = response.access_token {
        Ok((access_token, response.refresh_token.unwrap()))
    } else {
        Err(anyhow::Error::msg(response.error_description.unwrap()))
    }
}

#[debug_handler]
async fn handle_access_token(
    State(state): State<ServerState>,
    Query(query): Query<AuthQuery>,
) -> impl IntoResponse {
    if let Some(code) = query.code {
        state
            .tx
            .send(Authentication::AccessCode(code))
            .expect("Failed to send access code to main thread");

        Response::builder()
            .header("Content-Type", "text/html")
            .body("<html><body><h1>Authentication successful, you can now close this window.</h1></body></html>".to_string())
            .unwrap()
    } else {
        let error_description = query
            .error_description
            .unwrap_or("Unknown error".to_string());
        state
            .tx
            .send(Authentication::Error(error_description.clone()))
            .unwrap();

        Response::builder()
            .header("Content-Type", "text/html")
            .body(format!(
                "<html><body><h1>Authentication failed: {error_description}</h1></body></html>"
            ))
            .unwrap()
    }
}

pub async fn get_access_token() -> Result<(String, String)> {
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel::<Authentication>();
    let (stop_sender, stop_receiver) = oneshot::channel::<()>();
    tokio::spawn(async move {
        let listener = tokio::net::TcpListener::bind("0.0.0.0:0")
            .await
            .expect("Failed to start listening on 0.0.0.0");
        let port = listener.local_addr().unwrap().port();
        tx.send(Authentication::Port(port))
            .expect("Failed to send port to channel");
        let state = ServerState { tx };
        let router = axum::routing::Router::new()
            .route("/redirect", get(handle_access_token).with_state(state));
        let _ = axum::serve(listener, router)
            .with_graceful_shutdown(async {
                let _ = stop_receiver.await;
            })
            .await;
    });

    let mut server_port: Option<u16> = None;

    loop {
        match rx.recv().await.unwrap() {
            Authentication::Port(port) => {
                server_port = Some(port);
                let uri = get_authorization_code_url(port).unwrap();
                webbrowser::open(uri.as_str()).expect("Failed to launch browser");
            }
            Authentication::AccessCode(code) => {
                stop_sender.send(()).unwrap();
                let token = match access_token_from_authorization_code(server_port.unwrap(), code).await {
                    Ok(tokens) => tokens,
                    Err(error) => {
                        return Err(error)
                    }
                };
                return Ok(token);
            }
            Authentication::Error(error) => {
                stop_sender.send(()).unwrap();
                return Err(anyhow::Error::msg(error));
            }
        }
    }
}
