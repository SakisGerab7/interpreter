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
    Spawn,
    Yield,
    Sleep,

    Eof
};

struct Token {
    std::string value;
    size_t line;
    TokenType type;
    
    Token() = default;

    Token(TokenType type, std::string_view value, size_t line)
        : type(type), value(std::string(value)), line(line) {}

    friend std::ostream &operator<<(std::ostream &os, const Token &token) {
        return os << "Token(type: " << (int) token.type << ", value: " << token.value << ", line: " << token.line << ")";
    }
};

inline bool operator==(const Token &lhs, const Token &rhs) {
    return lhs.type == rhs.type && lhs.value == rhs.value && lhs.line == rhs.line;
}