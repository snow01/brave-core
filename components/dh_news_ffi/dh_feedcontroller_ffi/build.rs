fn main() {
    cxx_build::bridge("src/lib.rs")
        .file("src/blobstore.cc")
        .flag_if_supported("-std=c++14")
        .compile("dh_feedcontroller");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/blobstore.cc");
    println!("cargo:rerun-if-changed=include/blobstore.h");
}