#pragma once

#include "common.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"

struct Parser {
    Lexer &lexer;
    Token Curr, Prev;

    Parser(Lexer &lexer) : lexer(lexer), Curr(lexer.next_token()), Prev() {}

    std::vector<StmtPtr> parse();

private:
    StmtPtr declaration();
    StmtPtr var_declaration();
    StmtPtr func_declaration();
    StmtPtr struct_declaration();
    StmtPtr statement();
    StmtPtr disp_statement();
    StmtPtr block();
    StmtPtr if_statement();
    StmtPtr while_statement();
    StmtPtr for_statement();
    StmtPtr return_statement();
    StmtPtr expr_statement();

    std::vector<StmtPtr> block_statements();

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
    ExprPtr lambda_expression();

    void synchronize();

    bool match(TokenType type);

    template <typename... Args>
    bool match(Args... args);

    Token consume(TokenType type, const std::string &msg);

    bool check(TokenType type);

    Token advance();
    bool at_end();
    Token peek();
    Token previous();
    
};

struct ParseError : public std::runtime_error {
    ParseError(Token token, const std::string &msg)
        : std::runtime_error("[Parse Error] Line " + std::to_string(token.Line) + " at " + std::string(token.Value) + ": " + msg) {}
};