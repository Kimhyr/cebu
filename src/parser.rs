use crate::{
    diagnostics::{Position, SourceFile},
    syntax::{Token, TokenKind},
};
use std::{
    collections::{hash_map, HashMap},
    num::ParseIntError,
    str::Chars,
    sync::LazyLock,
};

static KEYWORD_MAP: LazyLock<HashMap<&'static str, TokenKind>> = LazyLock::new(|| {
    HashMap::from([
        ("fn", TokenKind::Fn),
        ("let", TokenKind::Let),
        ("ret", TokenKind::Ret),
        ("if", TokenKind::If),
        ("elif", TokenKind::Elif),
        ("else", TokenKind::Else),
        ("b8", TokenKind::B8),
        ("b16", TokenKind::B16),
        ("b32", TokenKind::B32),
        ("b64", TokenKind::B64),
        ("b128", TokenKind::B128),
        ("i8", TokenKind::I8),
        ("i16", TokenKind::I16),
        ("i32", TokenKind::I32),
        ("i64", TokenKind::I64),
        ("i128", TokenKind::I128),
        ("f16", TokenKind::F16),
        ("f32", TokenKind::F32),
        ("f64", TokenKind::F64),
        ("f128", TokenKind::F128),
    ])
});

#[derive(Debug)]
pub enum LexingError {
    UnknownToken,
    NumberParsing(ParseIntError),
    IncompleteString(Vec<char>),
    IncompleteCharacter,
    UnknownEscapeCharacter(char),
}

#[derive(Debug)]
pub struct Lexer<'a> {
    iterator: Chars<'a>,
    position: Position,
}

impl<'a> Lexer<'a> {
    pub fn new(iterator: Chars<'a>) -> Self {
        Self {
            position: Position { row: 1, column: 1 },
            iterator,
        }
    }

    pub fn position(&self) -> &Position {
        &self.position
    }

    fn consume(&mut self) -> Option<char> {
        match self.iterator.next() {
            None => None,
            Some(c) => {
                if c == '\n' {
                    self.position.row += 1;
                    self.position.column = 0;
                }
                self.position.column += 1;
                Some(c)
            }
        }
    }

    fn peek(&self) -> Option<char> {
        if let Some(c) = self.iterator.clone().peekable().peek() {
            Some(*c)
        } else {
            None
        }
    }

    fn buffered_lex(&mut self, mut current: char, check: fn(char) -> bool) -> String {
        let mut buffer = Vec::<char>::new();
        loop {
            buffer.push(current);
            current = match self.peek() {
                None => break,
                Some(c) => {
                    self.consume();
                    if check(c) {
                        c
                    } else {
                        break;
                    }
                }
            }
        }
        buffer.into_iter().collect()
    }

    fn peek_character(&mut self) -> Option<Result<(bool, char), LexingError>> {
        match self.peek() {
            None => None,
            Some('\\') => Some(Ok((
                true,
                match self.consume() {
                    Some(_) => match self.peek() {
                        Some(r) => match r {
                            '"' | '\\' | '\'' => r,
                            _ => return Some(Err(LexingError::UnknownEscapeCharacter(r))),
                        },
                        None => return None,
                    },
                    None => return None,
                },
            ))),
            Some(c) => Some(Ok((false, c))),
        }
    }
}

impl<'a> Iterator for Lexer<'a> {
    type Item = Result<Token, LexingError>;

    fn next(&mut self) -> Option<Self::Item> {
        let mut result: Option<char>;
        let mut current: char;
        loop {
            result = self.consume();
            if let None = result {
                return None;
            }
            current = result.unwrap();
            if !current.is_whitespace() {
                break;
            }
        }

        let mut token = Token::new(self.position.clone());
        token.kind = match current {
            '@' => TokenKind::CommercialAt,
            ':' => TokenKind::Colon,
            ';' => TokenKind::Semicolon,
            '=' => match self.peek() {
                Some('=') => {
                    self.consume();
                    TokenKind::DoubleEqualsSign
                }
                Some('>') => {
                    self.consume();
                    TokenKind::RightwardsDoubleArrow
                }
                _ => TokenKind::EqualsSign,
            },
            '+' => match self.peek() {
                Some('+') => {
                    self.consume();
                    TokenKind::DoublePlusSign
                }
                _ => TokenKind::PlusSign,
            },
            '-' => match self.peek() {
                Some('-') => {
                    self.consume();
                    TokenKind::DoubleMinusSign
                }
                Some('>') => {
                    self.consume();
                    TokenKind::RightwardsArrow
                }
                _ => TokenKind::MinusSign,
            },
            '|' => match self.peek() {
                Some('|') => {
                    self.consume();
                    TokenKind::DoubleVerticalLine
                }
                _ => TokenKind::VerticalLine,
            },
            '(' => TokenKind::LeftParenthesis,
            ')' => TokenKind::RightParenthesis,
            '"' => {
                let mut buffer = Vec::<char>::new();
                loop {
                    current = match self.peek_character() {
                        Some(Ok((is_escaped, c))) => {
                            if c == '"' && !is_escaped {
                                self.consume();
                                break;
                            }
                            c
                        }
                        Some(Err(e)) => return Some(Err(e)),
                        None => return Some(Err(LexingError::IncompleteString(buffer))),
                    };
                    buffer.push(current);
                    self.consume();
                }
                TokenKind::String(buffer.into_iter().collect())
            }
            '\'' => {
                let c = match self.peek_character() {
                    Some(Ok((_, c))) => c,
                    Some(Err(e)) => return Some(Err(e)),
                    None => return Some(Err(LexingError::IncompleteCharacter)),
                };
                self.consume();
                match self.peek() {
                    Some(c) => {
                        if c != '\'' {
                            self.consume();
                            return Some(Err(LexingError::IncompleteCharacter));
                        }
                    }
                    None => return Some(Err(LexingError::IncompleteCharacter)),
                }
                self.consume();
                TokenKind::Character(c)
            }
            c if c.is_digit(10) => {
                let buffer = self.buffered_lex(current, |c| c.is_digit(10));
                let parse = buffer.parse::<usize>();
                match parse {
                    Err(e) => return Some(Err(LexingError::NumberParsing(e))),
                    Ok(v) => TokenKind::Number(v),
                }
            }
            c if c.is_alphabetic() || c == '_' => {
                let buffer =
                    self.buffered_lex(current, |c| c.is_alphabetic() || c == '_' || c.is_digit(10));
                match KEYWORD_MAP.get(buffer.as_str()) {
                    None => TokenKind::Identifier(buffer),
                    Some(kw) => (*kw).clone(),
                }
            }
            _ => return Some(Err(LexingError::UnknownToken)),
        };

        Some(Ok(token))
    }
}

#[test]
fn test_lexer() {
    let tokens = [
        TokenKind::Identifier(String::from("_abc_123_")),
        TokenKind::Number(123),
        TokenKind::Character('a'),
        TokenKind::String(String::from(r#"123abd.\"'"#)),
        TokenKind::Fn,
        TokenKind::Let,
        TokenKind::Ret,
        TokenKind::If,
        TokenKind::Elif,
        TokenKind::Else,
        TokenKind::B8,
        TokenKind::B16,
        TokenKind::B32,
        TokenKind::B64,
        TokenKind::B128,
        TokenKind::I8,
        TokenKind::I16,
        TokenKind::I32,
        TokenKind::I64,
        TokenKind::I128,
        TokenKind::F16,
        TokenKind::F32,
        TokenKind::F64,
        TokenKind::F128,
        TokenKind::CommercialAt,
        TokenKind::Colon,
        TokenKind::Semicolon,
        TokenKind::DoubleEqualsSign,
        TokenKind::RightwardsDoubleArrow,
        TokenKind::EqualsSign,
        TokenKind::DoublePlusSign,
        TokenKind::PlusSign,
        TokenKind::DoubleMinusSign,
        TokenKind::RightwardsArrow,
        TokenKind::MinusSign,
        TokenKind::LeftParenthesis,
        TokenKind::RightParenthesis,
        TokenKind::DoubleVerticalLine,
        TokenKind::VerticalLine,
    ];
    let mut lexer = Lexer::new(
        r#"
        _abc_123_   
        123 'a' "123abd.\\\"\'"
        fn let ret if elif else
        b8 b16 b32 b64 b128
        i8 i16 i32 i64 i128
        f16 f32 f64 f128
        @:;===>=+++--->-()|||
    "#
        .chars(),
    );
    for token in tokens {
        match lexer.next() {
            Some(Ok(result)) => {
                println!("expected: {:?}, recieved: {:?}", token, result.kind());
                if *result.kind() != token {
                    panic!()
                }
            }
            Some(Err(e)) => {
                println!("unexpected error: {:?}", e);
                panic!()
            }
            None => {
                println!("expected token");
                panic!()
            }
        }
    }
}
