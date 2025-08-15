mod oidc;

// use crate::oidc::*;

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        // #[namespace = "tauros_cxx::oidc"]
        // fn oidc_access_token(issuer: String, loopback_port: i32, loopback_route: String) -> String;
    }
}
