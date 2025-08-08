#pragma once

#include "common.hpp"
#include "token.hpp"
#include "expr.hpp"
#include "stmt.hpp"

struct Parser {
    const std::vector<Token> &Tokens;
    size_t Curr = 0;
    
    Parser(const std::vector<Token> &tokens) : Tokens(tokens) {}

    std::vector<StmtPtr> parse();

    private:
    StmtPtr declaration();
    StmtPtr var_declaration();
    StmtPtr func_declaration();
    StmtPtr statement();
    StmtPtr disp_statement();
    StmtPtr block();
    StmtPtr if_statement();
    StmtPtr while_statement();
    StmtPtr for_statement();
    StmtPtr return_statement();
    StmtPtr expr_statement();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr ternary();
    ExprPtr logic_or();
    ExprPtr logic_and();
    ExprPtr bit_or();
    ExprPtr bit_xor();
    ExprPtr bit_and();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr bit_shift();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();
    ExprPtr array_literal();
    ExprPtr object_literal();

    void synchronize();

    template <typename... Args>
    bool match(Args... args);

    Token consume(TokenType type, const std::string &msg);

    bool check(TokenType type);

    Token advance();
    bool at_end();
    Token peek();
    Token previous();
};