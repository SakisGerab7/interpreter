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
struct CallExpr;
struct ArrayExpr;
struct ObjectExpr;
struct IndexExpr;
struct DotExpr;
struct TernaryExpr;
struct LambdaExpr;
struct SelfExpr;

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
    CallExpr,
    ArrayExpr,
    ObjectExpr,
    IndexExpr,
    DotExpr,
    TernaryExpr,
    LambdaExpr,
    SelfExpr
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
    ExprPtr Left, Right;
    Token Op;
};

struct LogicalExpr {
    ExprPtr Left, Right;
    Token Op;
};

struct UnaryExpr {
    ExprPtr Right;
    Token Op;
};

struct PostfixExpr {
    ExprPtr Left;
    Token Op;
};

struct GroupingExpr {
    ExprPtr Grouped;
};

struct LiteralExpr {
    Value Literal;
};

struct VariableExpr {
    Token Name;
};

struct AssignExpr {
    ExprPtr Target, Value;
    Token Op;
};

struct CallExpr {
    ExprPtr Callee;
    std::vector<ExprPtr> Args;
};

struct ArrayExpr {
    std::vector<ExprPtr> Elements;
};

struct ObjectExpr {
    std::unordered_map<std::string, ExprPtr> Items;
};

struct IndexExpr {
    ExprPtr Target, Index;
};

struct DotExpr {
    ExprPtr Target;
    Token Key;
};

struct TernaryExpr {
    ExprPtr Condition, Left, Right;
};

struct LambdaExpr {
    std::vector<Token> Params;
    std::vector<StmtPtr> Body;
};

struct SelfExpr {
    Token Keyword;
};

// ==== Statement structs ====
struct ExprStmt {
    ExprPtr Expr;
};

struct DispStmt {
    ExprPtr Expr;
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
    StmtPtr ThenBranch, ElseBranch;
};

struct WhileStmt {
    ExprPtr Condition;
    StmtPtr Body;
};

struct FunctionStmt {
    Token Name;
    std::vector<Token> Params;
    std::vector<StmtPtr> Body;
};

struct ReturnStmt {
    ExprPtr Value;
};

struct StructStmt {
    Token Name;
    std::vector<StmtPtr> Methods;
};