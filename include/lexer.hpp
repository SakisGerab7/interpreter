#pragma once

#include "common.hpp"
#include "token.hpp"

struct Lexer {
    std::string_view src;
    size_t start = 0, curr = 0, line = 1;

    Lexer(std::string_view source) : src(source) {}

    Token next_token();

private:
    void skip_multiline_comment();

    Token next_identifier();
    Token next_number();
    Token next_string();

    bool match(char c);

    inline Token token_from(TokenType type);
    inline Token token_from(TokenType type, std::string_view value);

    inline char peek();
    inline char peek_next();
    inline char advance();
    inline bool at_end();
};

struct LexerError : public std::runtime_error {
    LexerError(size_t line, const std::string &msg)
        : std::runtime_error("[Lexer error] Line " + std::to_string(line) + ": " + msg) {}
};