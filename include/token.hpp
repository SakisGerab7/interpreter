#pragma once

#include "common.hpp"

enum class TokenType {
    // Symbol tokens
    LeftParen,     // (
    RightParen,    // )
    LeftBracket,   // [
    RightBracket,  // ]
    LeftCurly,     // {
    RightCurly,    // }

    Comma,         // ,
    Dot,           // .
    Semicolon,     // ;
    Questionmark,  // ?
    Colon,         // :
    Arrow,         // ->
    
    Plus,          // +
    Minus,         // -
    Mult,          // *
    Div,           // /
    Mod,           // %
    Not,           // !
    And,           // &&
    Or,            // ||
    Greater,       // > 
    Less,          // <

    BitNot,        // ~
    BitAnd,        // &
    BitOr,         // |
    BitXor,        // ^
    BitShiftLeft,  // <<
    BitShiftRight, // >>
    
    Assign,        // =
    Equal,         // ==
    NotEqual,      // !=
    PlusEqual,     // +=
    MinusEqual,    // -=
    MultEqual,     // *=
    DivEqual,      // /=
    ModEqual,      // %=
    GreaterEqual,  // >=
    LessEqual,     // <=

    Increment,     // ++
    Decrement,     // --

    // Literal tokens
    Identifier,
    String,
    Integer,
    Float,

    // Keyword tokens
    Let,
    Struct,
    Function,
    True,
    False,
    For,
    In,
    While,
    If,
    Else,
    Null,
    Return,
    Self,
    Disp,

    Eof
};

struct Token {
    std::string_view Value;
    size_t Line;
    TokenType Type;

    Token() = default;

    Token(TokenType type, std::string_view value, size_t line)
        : Type(type), Value(value), Line(line) {}

    friend std::ostream &operator<<(std::ostream &os, const Token &token) {
        return os << "Token(type: " << (int) token.Type << ", value: " << token.Value << ", line: " << token.Line << ")";
    }
};

inline bool operator==(const Token &lhs, const Token &rhs) {
    return lhs.Type == rhs.Type && lhs.Value == rhs.Value && lhs.Line == rhs.Line;
}