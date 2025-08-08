#pragma once

#include "common.hpp"
#include "value.hpp"
#include "expr.hpp"

struct ExprStmt;
struct DispStmt;
struct LetStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct FunctionStmt;
struct ReturnStmt;

using Stmt = std::variant<
    ExprStmt,
    DispStmt,
    LetStmt,
    BlockStmt,
    IfStmt,
    WhileStmt,
    FunctionStmt,
    ReturnStmt
>;

using StmtPtr = std::unique_ptr<Stmt>;

template<typename T, typename... Args>
StmtPtr make_stmt(Args&&... args) {
    return std::make_unique<Stmt>(T{std::forward<Args>(args)...});
}

struct ExprStmt {
    ExprPtr expr;
};

struct DispStmt {
    ExprPtr expr;
};

struct LetStmt {
    Token Name;
    ExprPtr Initializer;
};

struct BlockStmt {
    std::vector<StmtPtr> Statements;
};

struct IfStmt {
    ExprPtr Condition;
    StmtPtr then_branch, else_branch;
};

struct WhileStmt {
    ExprPtr Condition;
    StmtPtr Body;
};

struct FunctionStmt {
    Token Name;
    std::vector<Token> Params;
    StmtPtr Body;
};

struct ReturnStmt {
    ExprPtr Val;
};
