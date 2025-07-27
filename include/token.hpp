#pragma once

#include "common.hpp"

enum class TokenType {
    // Symbol tokens
    LeftParen,    // (
    RightParen,   // )
    LeftBracket,  // [
    RightBracket, // ]
    LeftCurly,    // {
    RightCurly,   // }

    Comma,        // ,
    Dot,          // .
    Semicolon,    // ;
    
    Plus,         // +
    Minus,        // -
    Mult,         // *
    Div,          // /
    Mod,          // %
    Not,          // !
    And,          // &&
    Or,           // ||
    Greater,      // > 
    Less,         // <
    
    Assign,       // =
    Equal,        // ==
    NotEqual,     // !=
    PlusEqual,    // +=
    MinusEqual,   // -=
    MultEqual,    // *=
    DivEqual,     // /=
    ModEqual,     // %=
    GreaterEqual, // >=
    LessEqual,    // <=

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
    TokenType Type;
    std::string Value;
    size_t Line;

    Token(TokenType type, const std::string &value, size_t line)
        : Type(type), Value(value), Line(line) {}

    friend std::ostream &operator<<(std::ostream &os, const Token &token) {
        return os << "Token(type: " << (int) token.Type << ", value: " << token.Value << ", line: " << token.Line << ")";
    }
};

inline bool operator==(const Token &lhs, const Token &rhs) {
    return lhs.Type == rhs.Type && lhs.Value == rhs.Value && lhs.Line == rhs.Line;
}