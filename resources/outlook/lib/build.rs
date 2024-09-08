use cxx_qt_build::CxxQtBuilder;
use std::path::Path;

fn main() {
    CxxQtBuilder::new()
        .file("src/resource_state.rs")
        .build();
}
