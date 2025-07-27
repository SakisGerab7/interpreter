#pragma once

#include "common.hpp"
#include "token.hpp"

struct Lexer {
    std::string source;
    std::vector<Token> tokens;
    size_t start = 0, current = 0, line = 1;

    Lexer (const std::string &src) : source(src) {}

    std::vector<Token> tokenize();

private:
    void next_token();

    void skip_multiline_comment();

    void next_identifier();
    void next_number();
    void next_string();

    bool match(char c);

    void add_token(TokenType type);
    void add_token(TokenType type, const std::string& value);

    char peek();
    char peek_next();
    char advance();
    bool at_end();
};