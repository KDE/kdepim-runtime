use cxx_qt_build::CxxQtBuilder;
use std::path::Path;

fn main() {
    CxxQtBuilder::new()
        .file("src/resource_state.rs")
        .cc_builder(|cc| {
            cc.include(
                Path::new(std::env::var("CARGO_MANIFEST_DIR").unwrap().as_str())
                    .join("src")
                    .join("includes")
                    .display()
                    .to_string(),
            );
        })
        .build();
}
