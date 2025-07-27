#pragma once

#include "common.hpp"
#include "environment.hpp"

class Stmt;

class ExprStmt;
class DispStmt;
class LetStmt;
class BlockStmt;
class IfStmt;

class Expr;

class BinaryExpr;
class UnaryExpr;
class GroupingExpr;
class LiteralExpr;
class VariableExpr;
class AssignExpr;

class Value;

struct Interpreter {
    std::shared_ptr<Environment> Globals = std::make_unique<Environment>();
    std::shared_ptr<Environment> Curr = Globals;

    void interpret(const std::vector<std::unique_ptr<Stmt>> &statements);

    void execute(const Stmt &stmt);

    void execute_expr(const ExprStmt &stmt);
    void execute_disp(const DispStmt &stmt);
    void execute_let(const LetStmt &stmt);
    void execute_block(const BlockStmt &stmt);
    void execute_if(const IfStmt &stmt);

    Value evaluate(const Expr &expr);

    Value eval_binary(const BinaryExpr &expr);
    Value eval_unary(const UnaryExpr &expr);
    Value eval_variable(const VariableExpr &expr);
    Value eval_assignment(const AssignExpr &expr);
};