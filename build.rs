fn main() {
    cxx_build::bridge("ffi/index.rs")
        .std("c++20")
        .include("Source")
        .include("crates/cxx-async/cxx-async/include")
        .compile("tauros_cxx");

    println!("cargo:rerun-if-changed=ffi");
    println!("cargo:rerun-if-changed=build.rs");
}
