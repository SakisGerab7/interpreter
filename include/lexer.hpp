#pragma once

#include "common.hpp"
#include "token.hpp"

struct Lexer {
    std::string_view Src;
    std::vector<Token> &Tokens;
    size_t Start = 0, Curr = 0, Line = 1;

    Lexer(std::string_view src, std::vector<Token> &tokens) : Src(src), Tokens(tokens) {}

    void tokenize();

private:
    void next_token();

    void skip_multiline_comment();

    void next_identifier();
    void next_number();
    void next_string();

    bool match(char c);

    void add_token(TokenType type);
    void add_token(TokenType type, std::string_view value);

    char peek();
    char peek_next();
    char advance();
    bool at_end();
};