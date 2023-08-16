use crate::diagnostics::Position;

/// fn fib(n: u64) -> u64
///     = n == (0 || 1) => n,
///         fib(n - 1) + fib(n - 2);

#[derive(Debug)]
pub struct Token {
    position: Position,
    pub kind: TokenKind,
}

impl Token {
    pub fn new(position: Position) -> Self {
        Self {
            position,
            kind: TokenKind::None,
        }
    }

    pub fn kind(&self) -> &TokenKind {
        &self.kind
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum TokenKind {
    None,
    Identifier(String), // (<letter>|'_') +[<letter>|'_'|<whole-number>]

    // Literals
    Number(usize),   // +(<whole-number>)
    Character(char), // ''' <character> '''
    String(String),  // '"' +<character> '"'

    // Keywords
    Fn,   // 'fn'
    Let,  // 'let'
    Ret,  // 'ret'
    If,   // 'if'
    Elif, // 'elif'
    Else, // 'else'
    B8,   // 'b8'
    B16,  // 'b16'
    B32,  // 'b32'
    B64,  // 'b64'
    B128, // 'b128'
    I8,   // 'i8'
    I16,  // 'i16'
    I32,  // 'i32'
    I64,  // 'i64'
    I128, // 'i128'
    F16,  // 'f16'
    F32,  // 'f32'
    F64,  // 'f64'
    F128, // 'f128'

    // Punctuators
    CommercialAt,          // '@'
    Colon,                 // ':'
    Semicolon,             // ';'
    EqualsSign,            // '='
    DoubleEqualsSign,      // '=='
    RightwardsDoubleArrow, // '=>'
    PlusSign,              // '+'
    DoublePlusSign,        // '++'
    MinusSign,             // '-'
    DoubleMinusSign,       // '--'
    RightwardsArrow,       // '->'
    LeftParenthesis,       // '('
    RightParenthesis,      // ')'
    VerticalLine,          // '|'
    DoubleVerticalLine,    // '||'
}

#[derive(Debug)]
pub struct Symbol<'a> {
    name: String,
    syntax: &'a Syntax<'a>,
}

#[derive(Debug)]
pub struct Syntax<'a> {
    position: Position,
    kind: SyntaxKind<'a>,
}

#[derive(Debug)]
pub enum SyntaxKind<'a> {
    Statement(Statement<'a>),
}

/// <expression>|<body>|<function-declaration>|...
#[derive(Debug)]
pub enum Statement<'a> {
    Expression(Expression<'a>),
    Body(Body<'a>),

    /// 'fn' <function-definition>
    FunctionDeclaration {
        definition: FunctionDefinition<'a>,
    },
}

/// <block>|<initilization>
#[derive(Debug)]
pub enum Body<'a> {
    Block(Block<'a>),

    /// '=' <expression> ';'
    Initialization {
        value: Expression<'a>,
    },
}

/// '{' +[<statement>] '}'
#[derive(Debug)]
pub struct Block<'a> {
    statements: Vec<Statement<'a>>,
}

#[derive(Debug)]
pub enum Type {
    Primitive(PrimitiveType),
}

#[derive(Debug)]
pub enum PrimitiveType {
    B8,
    B16,
    B32,
    B64,
    B128,
    I8,
    I16,
    I32,
    I64,
    I128,
    F16,
    F32,
    F64,
    F128,
}

/// <block>|<equation>|<implication>|<disjunction>|<enclosure>|<invocation>
#[derive(Debug)]
pub enum Expression<'a> {
    Block(Block<'a>),

    /// <symbol> '=' <body>
    Equation {
        symbol: &'a Symbol<'a>,
        value: Box<Expression<'a>>,
    },

    /// <expression> '=>' <expression> ',' <expression>
    Implication {
        condition: Box<Expression<'a>>,
        consequence: Box<Expression<'a>>,
        contrapositive: Box<Expression<'a>>,
    },

    /// <expression> '||' <expression>
    Disjunction {
        left: Box<Expression<'a>>,
        right: Box<Expression<'a>>,
    },

    /// '(' <expression> ')'
    Enclosure {
        expression: Box<Expression<'a>>,
    },

    /// <symbol> <parameters>
    Invocation {
        arguments: Parameters<'a>,
    },
}

/// <function-signature> <body>
#[derive(Debug)]
pub struct FunctionDefinition<'a> {
    signature: FunctionSignature<'a>,
    body: Body<'a>,
}

/// <symbol> <parameters> '->' <type>
#[derive(Debug)]
pub struct FunctionSignature<'a> {
    name: &'a Symbol<'a>,
    parameters: Parameters<'a>,
    type_: &'a Type,
}

/// '(' <value-signature> +[',' <value-signature>] ')'
type Parameters<'a> = Vec<ValueDefinition<'a>>;

/// <value-signature> <body>
#[derive(Debug)]
pub struct ValueDefinition<'a> {
    signature: ValueSignature<'a>,
    value: Body<'a>,
}

/// <symbol> ':' <type>
#[derive(Debug)]
pub struct ValueSignature<'a> {
    symbol: &'a Symbol<'a>,
    type_: &'a Type,
}
