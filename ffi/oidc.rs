use axum::{Router, extract::Query, routing::get};
use openidconnect::core::*;
use openidconnect::{
    AuthenticationFlow, AuthorizationCode, ClientId, ClientSecret, CsrfToken, IssuerUrl, Nonce,
    PkceCodeChallenge, RedirectUrl, Scope,
};
use openidconnect::{OAuth2TokenResponse, reqwest};
use std::i32;
use std::sync::Arc;
use tokio::sync::{Mutex, oneshot};

pub async fn oidc_access_token(
    issuer: String,
    client_id: String,
    client_secret: Option<String>,
    loopback_port: i32,
    loopback_route: String,
) -> anyhow::Result<String> {
    let http_client = reqwest::ClientBuilder::new()
        .build()
        .expect("Client build error");

    let provider =
        CoreProviderMetadata::discover_async(IssuerUrl::new(issuer.to_string())?, &http_client)
            .await?;

    let client = CoreClient::from_provider_metadata(
        provider,
        ClientId::new(client_id.into()),
        client_secret.map(ClientSecret::new),
    )
    .set_redirect_uri(RedirectUrl::new(format!(
        "http://127.0.0.1:{}{}",
        loopback_port, loopback_route
    ))?);

    let (pkce_challenge, pkce_verifier) = PkceCodeChallenge::new_random_sha256();
    let (auth_url, state, _nonce) = client
        .authorize_url(
            AuthenticationFlow::<CoreResponseType>::AuthorizationCode,
            CsrfToken::new_random,
            Nonce::new_random,
        )
        .set_pkce_challenge(pkce_challenge)
        .add_scope(Scope::new("openid".into()))
        .url();

    webbrowser::open(auth_url.as_str())?;

    #[derive(serde::Deserialize)]
    #[allow(dead_code)]
    struct Params {
        code: String,
        state: String,
    }

    let (tx, rx) = oneshot::channel::<Params>();
    let (shutdown_tx, shutdown_rx) = oneshot::channel::<()>();

    let arc_tx = Arc::new(Mutex::new(Some(tx)));
    let arc_shutdown_tx = Arc::new(Mutex::new(Some(shutdown_tx)));

    let expected_state = state.secret().to_string();
    let app = Router::new().route(
        &loopback_route,
        get(move |Query(p): Query<Params>| {
            let shared_tx_clone = arc_tx.clone();
            let expected_state_clone = expected_state.clone();
            let shared_shutdown_tx_clone = arc_shutdown_tx.clone();

            async move {
                assert_eq!(p.state, expected_state_clone);

                if p.state != expected_state_clone {
                    return "State does not match.";
                }

                if let Some(sender) = shared_tx_clone.lock().await.take() {
                    let _ = sender.send(p);

                    if let Some(shutdown) = shared_shutdown_tx_clone.lock().await.take() {
                        let _ = shutdown.send(());
                    }
                    "You can close this window."
                } else {
                    "This request has already been processed."
                }
            }
        }),
    );

    let listener = tokio::net::TcpListener::bind(format!("127.0.0.1:{}", loopback_port)).await?;
    axum::serve(listener, app)
        .with_graceful_shutdown(async {
            shutdown_rx.await.ok();
        })
        .await?;

    let params = rx.await?;
    let code = params.code;

    let token_response = client
        .exchange_code(AuthorizationCode::new(code))?
        .set_pkce_verifier(pkce_verifier)
        .request_async(&http_client)
        .await?;

    Ok(token_response.access_token().secret().to_string())
}
// 
// use crate::ffi::OidcResult;
// 
// pub fn oidc_access_token_callback(
//     issuer: String,
//     client_id: String,
//     client_secret: String,
//     loopback_port: i32,
//     loopback_route: String,
//     callback: fn(result: OidcResult),
// ) {
//     tokio::spawn(async move {
//         let result = match oidc_access_token(
//             issuer,
//             client_id,
//             (!client_secret.trim().is_empty()).then_some(client_secret),
//             loopback_port.into(),
//             loopback_route,
//         )
//         .await
//         {
//             Ok(access_token) => OidcResult {
//                 bSuccess: true,
//                 AccessToken: access_token,
//                 ErrorMessage: "".to_string(),
//             },
//             Err(e) => OidcResult {
//                 bSuccess: false,
//                 AccessToken: "".to_string(),
//                 ErrorMessage: e.to_string(),
//             },
//         };
// 
//         callback(result);
//     });
// }
