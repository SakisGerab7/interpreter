#pragma once

#include "common.hpp"
#include "environment.hpp"

class Stmt;

class ExprStmt;
class DispStmt;
class LetStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;

class Expr;

class BinaryExpr;
class LogicalExpr;
class UnaryExpr;
class GroupingExpr;
class LiteralExpr;
class VariableExpr;
class AssignExpr;
class CallExpr;
class ArrayExpr;
class IndexExpr;

class Value;

struct Interpreter {
    std::shared_ptr<Environment> Globals = std::make_unique<Environment>();
    std::shared_ptr<Environment> Curr = Globals;

    Interpreter();

    void interpret(const std::vector<std::unique_ptr<Stmt>> &statements);

    void execute(const Stmt &stmt);

    void execute_expr(const ExprStmt &stmt);
    void execute_disp(const DispStmt &stmt);
    void execute_let(const LetStmt &stmt);
    void execute_block(const BlockStmt &stmt);
    void execute_if(const IfStmt &stmt);
    void execute_while(const WhileStmt &stmt);

    Value evaluate(const Expr &expr);

    Value eval_binary(const BinaryExpr &expr);
    Value eval_logical(const LogicalExpr &expr);
    Value eval_unary(const UnaryExpr &expr);
    Value eval_variable(const VariableExpr &expr);
    Value eval_assignment(const AssignExpr &expr);
    Value eval_call(const CallExpr &expr);
    Value eval_array(const ArrayExpr &expr);
    Value eval_index(const IndexExpr &expr);
};