use std::env;
use std::path::PathBuf;

use bindgen::callbacks::ParseCallbacks;
use bindgen::EnumVariation;
use cmake::Config;
use regex::Regex;

fn main() {
    let native_dir = concat!(env!("CARGO_MANIFEST_DIR"), "/hardcore");
    let mut config = Config::new(native_dir);
    config
        .define("VERSION_MAJOR", env!("CARGO_PKG_VERSION_MAJOR"))
        .define("VERSION_MINOR", env!("CARGO_PKG_VERSION_MINOR"))
        .define("VERSION_PATCH", env!("CARGO_PKG_VERSION_PATCH"))
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("GLFW_BUILD_EXAMPLES", "OFF")
        .define("GLFW_BUILD_TESTS", "OFF")
        .define("GLFW_BUILD_DOCS", "OFF")
        .define("GLFW_INSTALL", "OFF");

    #[cfg(feature = "headless")]
    {
        config.define("HARDCORE_HEADLESS", "ON");
    }

    #[cfg(feature = "no_logs")]
    {
        config.define("HARDCORE_LOGS", "OFF");
    }

    #[cfg(feature = "validation")]
    {
        config.define("HARDCORE_VALIDATION", "ON");
    }

    let lib_path = config.build();
    println!(
        "cargo:rustc-link-search=native={}",
        lib_path.join("lib").display()
    );
    println!("cargo:rustc-link-lib=static=hardcore");

    #[cfg(target_family = "windows")]
    {
        println!("cargo:rustc-link-lib=dylib=gdi32");
        println!("cargo:rustc-link-lib=dylib=shell32");
        println!("cargo:rustc-link-lib=dylib=user32");
    }

    let bindings = bindgen::Builder::default()
        .header(
            lib_path
                .join("include")
                .join("hardcore.h")
                .to_string_lossy(),
        )
        .allowlist_function("hc_.*")
        .allowlist_type("HC.*")
        .allowlist_var("HC_.*")
        .default_enum_style(EnumVariation::Rust {
            non_exhaustive: true,
        })
        .rustified_enum("HCLogKind") // LogKind gets passed back through an extern function, so can't be non-exhaustive
        .parse_callbacks(Box::<StripPrefixCallback>::default())
        .use_core()
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

#[derive(Debug)]
struct StripPrefixCallback {
    fn_regex: Regex,
    type_const_regex: Regex,
    param_regex: Regex,
}

impl Default for StripPrefixCallback {
    fn default() -> Self {
        Self {
            fn_regex: Regex::new(r"`hc_((?:\w|\d)*)(?:\(\))?`").unwrap(),
            type_const_regex: Regex::new(r"`HC_?((?:\w|\d)*(?:::(?:\w|\d)*)*)`").unwrap(),
            param_regex: Regex::new(r"@param ((?:\w|\d)*)(?:\s-)?\s").unwrap(),
        }
    }
}

impl ParseCallbacks for StripPrefixCallback {
    fn item_name(&self, original_item_name: &str) -> Option<String> {
        let prefixes = ["hc_", "HC_", "HC"];
        for prefix in prefixes {
            let new_name = original_item_name.strip_prefix(prefix);
            if new_name.is_some() {
                return new_name.map(str::to_string);
            }
        }
        None
    }

    fn process_comment(&self, comment: &str) -> Option<String> {
        let mut rustified = if let Some(stripped) = comment.strip_prefix("!< ") {
            stripped.to_string()
        } else {
            comment.to_string()
        };

        let substitutions = [("@brief ", ""), ("@return ", "# Returns\n")];
        for (original, replacement) in substitutions {
            rustified = rustified.replace(original, replacement);
        }

        rustified = self.fn_regex.replace_all(&rustified, "[`$1`]").to_string();
        rustified = self
            .type_const_regex
            .replace_all(&rustified, "[`$1`]")
            .to_string();

        if let Some(idx) = rustified.find("@param") {
            rustified.insert_str(idx, "# Arguments\n");
            rustified = self
                .param_regex
                .replace_all(&rustified, "* `$1` - ")
                .to_string();
        }

        Some(rustified)
    }
}
