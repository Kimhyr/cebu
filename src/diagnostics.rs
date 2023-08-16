use std::fs::File;

#[derive(Debug, PartialEq, Clone)]
pub struct Position {
    pub row: usize,
    pub column: usize,
}

#[derive(Debug)]
pub struct SourceFile<'a> {
    file: File,
    path: &'a str,
}
