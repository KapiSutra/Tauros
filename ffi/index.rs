use std::pin::Pin;

mod oidc;

unsafe extern "C" {
    fn tokio_rt();
}

#[cxx::bridge]
pub mod ffi {
    pub struct OidcResult {
        bSuccess: bool,
        AccessToken: String,
        ErrorMessage: String,
    }

    unsafe extern "C++" {
        include!("Tauros/Oidc/TaurosOidcBridge.h");
        type RustFutureString = crate::RustFutureString;
    }

    extern "Rust" {
        #[namespace = "tauros_cxx::oidc"]
        fn oidc_access_token(
            issuer: String,
            client_id: String,
            client_secret: String,
            loopback_port: i32,
            loopback_route: String,
        ) -> RustFutureString;
    }
}

#[cxx_async::bridge]
unsafe impl Future for RustFutureString {
    type Output = String;
}

fn oidc_access_token(
    issuer: String,
    client_id: String,
    client_secret: String,
    loopback_port: i32,
    loopback_route: String,
) -> RustFutureString {
    RustFutureString::infallible(async move {
        let result = oidc::oidc_access_token(
            issuer,
            client_id,
            (!client_secret.trim().is_empty()).then_some(client_secret),
            loopback_port,
            loopback_route,
        )
        .await;

        result.unwrap_or_else(|_| String::new())
    })
}
