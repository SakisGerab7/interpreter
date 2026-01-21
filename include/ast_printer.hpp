#pragma once

#include "common.hpp"
#include "ast.hpp"

struct AstPrinter {
    struct IndentGuard {
        size_t &level;
        IndentGuard(size_t &level) : level(level) { level++; }
        ~IndentGuard() { level--; }
    };

    size_t indent_level = 0;

    std::string indent() const;

    std::string print(const Expr &expr);
    std::string print(const Stmt &stmt);
    
    std::string print_binary(const BinaryExpr &expr);
    std::string print_logical(const LogicalExpr &expr);
    std::string print_unary(const UnaryExpr &expr);
    std::string print_postfix(const PostfixExpr &expr);
    std::string print_grouping(const GroupingExpr &expr);
    std::string print_literal(const LiteralExpr &expr);
    std::string print_variable(const VariableExpr &expr);
    std::string print_assign(const AssignExpr &expr);
    std::string print_set_dot(const SetDotExpr &expr);
    std::string print_set_index(const SetIndexExpr &expr);
    std::string print_call(const CallExpr &expr);
    std::string print_array(const ArrayExpr &expr);
    std::string print_object(const ObjectExpr &expr);
    std::string print_index(const IndexExpr&expr);
    std::string print_dot(const DotExpr &expr);
    std::string print_ternary(const TernaryExpr &expr);
    std::string print_lambda(const LambdaExpr &expr);
    std::string print_self(const SelfExpr &expr);
    std::string print_spawn(const SpawnExpr &expr);

    std::string print_expr(const ExprStmt &stmt);
    std::string print_disp(const DispStmt &stmt);
    std::string print_let(const LetStmt &stmt);
    std::string print_function(const FunctionStmt &stmt);
    std::string print_block(const BlockStmt &stmt);
    std::string print_if(const IfStmt &stmt);
    std::string print_while(const WhileStmt &stmt);
    std::string print_foreach(const ForEachStmt &stmt);
    std::string print_return(const ReturnStmt &stmt);
    std::string print_struct(const StructStmt &stmt);
    std::string print_close(const CloseStmt &stmt);
    std::string print_select(const SelectStmt &stmt);


private:
    template<typename... Args>
    std::string parenthesize(const std::string &name, const Args... args);
    
    std::string parenthesize(const std::string &name, const std::vector<const Expr *> &exprs);
};