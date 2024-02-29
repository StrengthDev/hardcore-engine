use std::env;
use std::path::PathBuf;

use cmake::Config;

fn main() {
    let native_dir = concat!(env!("CARGO_MANIFEST_DIR"), "/hardcore");
    let mut config = Config::new(native_dir);

    let lib_path = config.build();
    println!("cargo:rustc-link-search=native={}/lib", lib_path.display());
    println!("cargo:rustc-link-lib=static=hardcore");

    let bindings = bindgen::Builder::default()
        .header(
            lib_path
                .join("include")
                .join("hardcore.h")
                .to_string_lossy(),
        )
        .parse_callbacks(Box::new(
            bindgen::CargoCallbacks::new().rerun_on_header_files(false),
        ))
        .allowlist_function("hc_.*")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
