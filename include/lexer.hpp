#pragma once

#include "common.hpp"
#include "token.hpp"

struct Lexer {
    std::string_view Src;
    size_t Start = 0, Curr = 0, Line = 1;

    Lexer(std::string_view src) : Src(src) {}

    Token next_token();

private:
    void skip_multiline_comment();

    Token next_identifier();
    Token next_number();
    Token next_string();

    bool match(char c);

    Token token_from(TokenType type);
    Token token_from(TokenType type, std::string_view value);

    char peek();
    char peek_next();
    char advance();
    bool at_end();
};

struct LexerError : public std::runtime_error {
    LexerError(size_t line, const std::string &msg)
        : std::runtime_error("[Lexer error] Line " + std::to_string(line) + ": " + msg) {}
};