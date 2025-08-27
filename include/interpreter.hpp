#pragma once

#include "common.hpp"
#include "environment.hpp"
#include "ast.hpp"

class Value;

struct Interpreter {
    std::shared_ptr<Environment> Globals = std::make_unique<Environment>();
    std::shared_ptr<Environment> Curr = Globals;

    std::unordered_map<std::string_view, Value> ArrayMethods;

    Interpreter();

    void interpret(const std::vector<std::unique_ptr<Stmt>> &statements);

    void execute(const Stmt &stmt);

    void execute_expr(const ExprStmt &stmt);
    void execute_disp(const DispStmt &stmt);
    void execute_let(const LetStmt &stmt);
    void execute_function(const FunctionStmt &stmt);
    void execute_block(const BlockStmt &stmt);
    void execute_if(const IfStmt &stmt);
    void execute_while(const WhileStmt &stmt);
    void execute_return(const ReturnStmt &stmt);
    void execute_struct(const StructStmt &stmt);

    Value evaluate(const Expr &expr);

    Value eval_binary(const BinaryExpr &expr);
    Value eval_logical(const LogicalExpr &expr);
    Value eval_unary(const UnaryExpr &expr);
    Value eval_postfix(const PostfixExpr &expr);
    Value eval_grouping(const GroupingExpr &expr);
    Value eval_literal(const LiteralExpr &expr);
    Value eval_variable(const VariableExpr &expr);
    Value eval_assignment(const AssignExpr &expr);
    Value eval_call(const CallExpr &expr);
    Value eval_array(const ArrayExpr &expr);
    Value eval_object(const ObjectExpr &expr);
    Value eval_index(const IndexExpr &expr);
    Value eval_dot(const DotExpr &expr);
    Value eval_ternary(const TernaryExpr &expr);
    Value eval_lambda(const LambdaExpr &expr);
    Value eval_self(const SelfExpr &expr);

    void assign_to_target(const Expr *target_expr, const Value &val);
    void assign_index(Value &container, const Value &index, const Value &val);

    Value call_function(std::shared_ptr<Callable> callable, const std::vector<Value> &args);

    std::shared_ptr<Callable> bind_instance(std::shared_ptr<StructInstance> inst, std::shared_ptr<Callable> method);

    struct EnterEnvironmentGuard {
        Interpreter &Interp;
        std::shared_ptr<Environment> Prev;

        EnterEnvironmentGuard(Interpreter &interp, std::shared_ptr<Environment> new_env)
            : Interp(interp), Prev(interp.Curr) { Interp.Curr = std::move(new_env); }

        ~EnterEnvironmentGuard() { Interp.Curr = Prev; }

        EnterEnvironmentGuard(const EnterEnvironmentGuard &) = delete;
        EnterEnvironmentGuard &operator=(const EnterEnvironmentGuard &) = delete;
    };
};