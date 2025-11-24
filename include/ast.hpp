#pragma once

#include "common.hpp"
#include "value.hpp"
#include "token.hpp"

// ==== Forward declarations for expressions and statements ====
struct BinaryExpr;
struct LogicalExpr;
struct UnaryExpr;
struct PostfixExpr;
struct GroupingExpr;
struct LiteralExpr;
struct VariableExpr;
struct AssignExpr;
struct SetDotExpr;
struct SetIndexExpr;
struct CallExpr;
struct ArrayExpr;
struct ObjectExpr;
struct IndexExpr;
struct DotExpr;
struct TernaryExpr;
struct LambdaExpr;
struct SelfExpr;
struct SpawnExpr;

struct ExprStmt;
struct DispStmt;
struct LetStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct FunctionStmt;
struct ReturnStmt;
struct StructStmt;

// ==== Type aliases ====
using Expr = std::variant<
    BinaryExpr,
    LogicalExpr,
    UnaryExpr,
    PostfixExpr,
    GroupingExpr,
    LiteralExpr,
    VariableExpr,
    AssignExpr,
    SetDotExpr,
    SetIndexExpr,
    CallExpr,
    ArrayExpr,
    ObjectExpr,
    IndexExpr,
    DotExpr,
    TernaryExpr,
    LambdaExpr,
    SelfExpr,
    SpawnExpr
>;

using ExprPtr = std::unique_ptr<Expr>;

using Stmt = std::variant<
    ExprStmt,
    DispStmt,
    LetStmt,
    BlockStmt,
    IfStmt,
    WhileStmt,
    FunctionStmt,
    ReturnStmt,
    StructStmt
>;

using StmtPtr = std::unique_ptr<Stmt>;

// ==== Expression factory ====
template <typename T, typename... Args>
ExprPtr make_expr(Args &&...args) {
    return std::make_unique<Expr>(T{std::forward<Args>(args)...});
}

// ==== Statement factory ====
template <typename T, typename... Args>
StmtPtr make_stmt(Args &&...args) {
    return std::make_unique<Stmt>(T{std::forward<Args>(args)...});
}

// ==== Expression structs ====
struct BinaryExpr {
    ExprPtr left, right;
    Token op;
};

struct LogicalExpr {
    ExprPtr left, right;
    Token op;
};

struct UnaryExpr {
    ExprPtr right;
    Token op;
};

struct PostfixExpr {
    ExprPtr left;
    Token op;
};

struct GroupingExpr {
    ExprPtr grouped;
};

struct LiteralExpr {
    Value literal;
};

struct VariableExpr {
    Token name;
};

struct AssignExpr {
    Token name;      // variable
    ExprPtr value;   // assigned value
    Token op;        // =, +=, etc.
};

struct SetDotExpr {
    ExprPtr target;
    Token key;
    ExprPtr value;
    Token op;        // =, +=, etc.
};

struct SetIndexExpr {
    ExprPtr target;
    ExprPtr index;
    ExprPtr value;
    Token op;        // =, +=, etc.
};

struct CallExpr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
};

struct ArrayExpr {
    std::vector<ExprPtr> elements;
};

struct ObjectExpr {
    std::unordered_map<std::string, ExprPtr> items;
};

struct IndexExpr {
    ExprPtr target, index;
};

struct DotExpr {
    ExprPtr target;
    Token key;
};

struct TernaryExpr {
    ExprPtr condition, left, right;
};

struct LambdaExpr {
    std::vector<Token> params;
    std::shared_ptr<std::vector<StmtPtr>> body;
};

struct SelfExpr {
    Token keyword;
};

struct SpawnExpr {
    ExprPtr count;
    std::shared_ptr<std::vector<StmtPtr>> statements;
};

// ==== Statement structs ====
struct ExprStmt {
    ExprPtr expr;
};

struct DispStmt {
    ExprPtr expr;
};

struct LetStmt {
    Token name;
    ExprPtr initializer;
};

struct BlockStmt {
    std::shared_ptr<std::vector<StmtPtr>> statements;
};

struct IfStmt {
    ExprPtr condition;
    StmtPtr then_branch, else_branch;
};

struct WhileStmt {
    ExprPtr condition;
    StmtPtr body;
};

struct FunctionStmt {
    Token name;
    std::vector<Token> params;
    std::shared_ptr<std::vector<StmtPtr>> body;
};

struct ReturnStmt {
    ExprPtr value;
};

struct StructStmt {
    Token name;
    std::vector<StmtPtr> methods;
};