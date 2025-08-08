#pragma once

#include "common.hpp"
#include "value.hpp"
#include "token.hpp"

struct BinaryExpr;
struct LogicalExpr;
struct UnaryExpr;
struct GroupingExpr;
struct LiteralExpr;
struct VariableExpr;
struct AssignExpr;
struct CallExpr;
struct ArrayExpr;
struct ObjectExpr;
struct IndexExpr;
struct TernaryExpr;

using Expr = std::variant<
    BinaryExpr,
    LogicalExpr,
    UnaryExpr,
    GroupingExpr,
    LiteralExpr,
    VariableExpr,
    AssignExpr,
    CallExpr,
    ArrayExpr,
    ObjectExpr,
    IndexExpr,
    TernaryExpr
>;

using ExprPtr = std::unique_ptr<Expr>;

template<typename T, typename... Args>
ExprPtr make_expr(Args&&... args) {
    return std::make_unique<Expr>(T{std::forward<Args>(args)...});
}

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

struct GroupingExpr {
    ExprPtr Grouped;
};

struct LiteralExpr {
    Value Lit;
};

struct VariableExpr {
    Token Name;
};

struct AssignExpr {
    ExprPtr Target, Val;
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

struct TernaryExpr {
    ExprPtr Condition, Left, Right;
};