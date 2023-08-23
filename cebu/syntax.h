#pragma once
#define CEBU_INCLUDED_SYNTAX_H

#include <cebu/diagnostics.h>
#include <cebu/token.h>
#include <cebu/utilities/type_traits.h>

namespace cebu
{

/// Program
///
/// # Syntax
///
/// program -> +declaration
class program;

///
class identifier;

/// Statements
///
/// # Syntax
///
/// statement -> expression ';'
///            | declaration
class statement;

/// Expressions
///
/// # Syntax
///
/// expression -> binary
///             | integer
///             | decimal
///             | character
///             | string
///             | parenthesized
///             | path
///             | invocation
///             | cast
///             | addition
///             | subtraction
///             | disjunction
///             | implication
///             | equation
/// parenthesized -> '(' expression ')'
/// path -> name +['::' name]
/// invocation -> path '(' mapping +[',' mapping] ')'
/// cast -> path ':' type
/// addition -> expression '+' expression
/// subtraction -> expression '-' expression
/// equation -> expressiong '==' expression
/// disjunction -> expression '|' expression
/// implication -> expression '=>' expression ',' expression
/// assignment -> path '=' expression
class expression;
class binary;
class integer;
class decimal;
class character;
class string;
class parenthesized;
class path;
class invocation;
class cast;
class addition;
class subtraction;
class disjunction;
class equation;
class implication;
class assignment;

/// Declarations - A referenceable grammar.
///
/// # Syntax
///
/// method-declaration -> ['method'] identifier lambda-type body
/// value-declaration -> ['value'] identifier ':' type body
class declaration;
class method_declaration;
class value_declaration;

/// Types
///
/// type -> primitive-type
///       | tuple-type
///       | lambda-type
/// primitive-type -> ('b'|'i')('8'|'16'|'32'|'64')
///                 | 'f'('16'|'32'|'64')
/// tuple-type -> '(' (value-declaration|type) +[',' (value-declaration|type)] ')'
/// lambda-type -> tuple-type '->' type
class type;
enum class primitive_type;
class tuple_type;
class lambda_type;

/// Parts - A part of a whole syntax.
///
/// # Syntax
///
/// mapping -> name ':' expression
/// body -> '=' expression ';'
///       | '{' +[statement] '}'
///       | ';'
class mapping;
class body;

class declaration
{
public:
    enum type_t
    {
        method,
        value_
    };

    operator method_declaration*&() { return value.method; }
    operator value_declaration*&()  { return value.value; }

    union {
        method_declaration* method;
        value_declaration*  value;
    }         value;
    type_t    type;
    std::byte padding[[maybe_unused]][4];
};

class identifier
{
public:
    std::string_view name;
};

class body
{
public:
    std::vector<statement> statements;
};

class type
{
public:
    enum type_t
    {
        primitive,
        tuple,
        lambda
    };

    operator primitive_type&() { return value.primitive; }
    operator tuple_type*&()    { return value.tuple; }
    operator lambda_type*&()   { return value.lambda; }

    union {
        primitive_type primitive;
        tuple_type*    tuple;
        lambda_type*   lambda;
    }      value;
    type_t type;
};

template<int Precedence>
class basic_expression
{
public:
    static constexpr int precedence = Precedence;
};

class path
    : public basic_expression<1>
{
public:
    std::vector<identifier> value;
};

class cast
    : public basic_expression<3>
{
public:
    path path;
    type type;
};

class basic_declaration
{
public:
    identifier identifier;
};

class value_declaration
    : public basic_declaration
{
public:
    type       type;
    body       body;
};

enum class primitive_type
{
    b8 = static_cast<int>(token_type::b8),
    b16,
    b32,
    b64,
    i8,
    i16,
    i32,
    i64,
    f16,
    f32,
    f64
};

class tuple_type
{
public:
    std::vector<value_declaration> mappings;
};

class lambda_type
{
public:
    tuple_type tuple;
    type       return_type;
};

/// 
/// let foo: i32 { if 32 == 2 { return 0; } else return 3; }
///

class expression
{
public:
    using precedence_t = unsigned;

    enum type_t
    {
        // 0
        integer,
        decimal,
        character,
        string,

        // 1
        parenthesized,
        path,

        // 2
        invocation,

        // 3
        cast,
        
        // 6
        addition,
        subtraction,

        // 10
        equation,

        // 15
        disjunction,

        // 16
        implication,
        assignment
    };

    operator cebu::path*&()        { return value.path; }
    operator cebu::invocation*&()  { return value.invocation; }
    operator cebu::cast*&()        { return value.cast; }
    operator cebu::addition*&()    { return value.addition; }
    operator cebu::subtraction*&() { return value.subtraction; }
    operator cebu::disjunction*&() { return value.disjunction; }
    operator cebu::implication*&() { return value.implication; }
    operator cebu::equation*&()    { return value.equation; }

    union {
        cebu::binary*        binary;
        cebu::integer*       integer;
        cebu::character*     character;
        cebu::string*        string;
        cebu::parenthesized* parenthesized;
        cebu::path*          path;
        cebu::invocation*    invocation;
        cebu::cast*          cast;
        cebu::addition*      addition;
        cebu::subtraction*   subtraction;
        cebu::disjunction*   disjunction;
        cebu::implication*   implication;
        cebu::equation*      equation;
    }      value;
    type_t type;
};

class statement
{
public:
    enum type_t
    {
        expression,
        declaration
    };

    operator cebu::expression&()  { return value.expression; }
    operator cebu::declaration&() { return value.declaration; }

    union {
        cebu::expression  expression;
        cebu::declaration declaration;
    }      value;
    type_t type;
};

class program
{
public:
    std::vector<declaration> declarations;
};

template<typename Value>
class basic_literal
    : public basic_expression<0>
{
public:
    Value value;  
};

class binary         : public basic_literal<std::uint64_t> {};
class integer        : public basic_literal<std::int64_t> {};
class decimal: public basic_literal<double> {};
class character      : public basic_literal<char> {};
class string         : public basic_literal<std::vector<char>> {};

class parenthesized
    : public basic_expression<1>
{
public:
    cebu::expression expression;
};

class invocation
    : public basic_expression<2>
{
public:   
    path                 path;
    std::vector<mapping> arguments;
};

template<int Precedence>
class basic_binary_expression
    : public basic_expression<Precedence>
{
public:
    expression left;
    expression right;
};

class addition    : public basic_binary_expression<6> {};
class subtraction : public basic_binary_expression<6> {};
class equation    : public basic_expression<10> {};
class disjunction : public basic_expression<15> {};

class implication
    : public basic_expression<16>
{
public:
    expression condition;
    expression consequence;
    expression contrapositive;
};

class assignment
    : public basic_expression<16>
{
public:
    path path;
    body body;
};

class method_declaration
    : public basic_declaration
{
public:
    lambda_type lambda;
    body        body;
};

}
