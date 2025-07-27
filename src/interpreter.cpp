#include "interpreter.hpp"
#include "expr.hpp"
#include "token.hpp"
#include "stmt.hpp"

void Interpreter::interpret(const std::vector<StmtPtr> &statements) {
    try {
        for (const auto &stmt : statements) {
            execute(*stmt);
        }
    } catch (std::runtime_error e) {
        throw e;
    }
}

void Interpreter::execute(const Stmt &stmt) {
    stmt.exec(*this);
}

void Interpreter::execute_expr(const ExprStmt &stmt) {
    evaluate(*stmt.expr);
}

void Interpreter::execute_disp(const DispStmt &stmt) {
    std::cout << evaluate(*stmt.expr).to_string() << "\n";
}

void Interpreter::execute_let(const LetStmt &stmt) {
    Value initializer;
    if (stmt.Initializer) {
        initializer = evaluate(*stmt.Initializer);
    }

    Curr->define(stmt.Name.Value, initializer);
}

void Interpreter::execute_block(const BlockStmt &stmt) {
    std::shared_ptr<Environment> prev = Curr;
    Curr = std::make_shared<Environment>(Curr.get());

    try {
        for (const StmtPtr &s : stmt.Statements) {
            s->exec(*this);
        }
        
        Curr = prev;
    } catch (...) {
        Curr = prev;
        throw;
    }
}

void Interpreter::execute_if(const IfStmt &stmt) {
    if (evaluate(*stmt.condition).is_truthy()) {
        execute(*stmt.then_branch);
    } else if (stmt.else_branch) {
        execute(*stmt.else_branch);
    }
}

Value Interpreter::evaluate(const Expr &expr) {
    return expr.eval(*this);
}

Value Interpreter::eval_binary(const BinaryExpr &expr) {
    Value left = evaluate(*expr.Left);
    Value right = evaluate(*expr.Right);

    switch (expr.Op.Type) {
        case TokenType::Plus:
            if ((left.is_int() || left.is_float()) && (right.is_int() || right.is_float())) {
                if (left.is_float() || right.is_float())
                    return left.as_float() + right.as_float();
                else
                    return left.as_int() + right.as_int();
            }

            return left.to_string() + right.to_string();
        case TokenType::Minus:
            if (left.is_float() || right.is_float())
                return left.as_float() - right.as_float();
            else
                return left.as_int() - right.as_int();
        case TokenType::Mult:
            if (left.is_float() || right.is_float())
                return left.as_float() * right.as_float();
            else
                return left.as_int() * right.as_int();
        case TokenType::Div:
            return left.as_float() - right.as_float();
        case TokenType::Mod:
            return static_cast<int>(left.as_int() % right.as_int());
        case TokenType::Equal:        return left == right;
        case TokenType::NotEqual:     return !(left == right);
        case TokenType::Greater:      return left.as_float() > right.as_float();
        case TokenType::GreaterEqual: return left.as_float() >= right.as_float();
        case TokenType::Less:         return left.as_float() < right.as_float();
        case TokenType::LessEqual:    return left.as_float() <= right.as_float();
    }

    throw std::runtime_error("Unknown binary operator: " + expr.Op.Value);
}

Value Interpreter::eval_unary(const UnaryExpr &expr) {
    Value right = evaluate(*expr.Right);

    if (expr.Op.Type == TokenType::Not) {
        return !right.is_truthy();
    } else if (expr.Op.Type == TokenType::Minus) {
        if (right.is_int())   return -right.as_int();
        if (right.is_float()) return -right.as_float();
        throw std::runtime_error("Unary '-' requires a number");
    }

    throw std::runtime_error("Unknown unary operator: " + expr.Op.Value);
}

Value Interpreter::eval_variable(const VariableExpr &expr) {
    return Curr->get(expr.Name);
}

Value Interpreter::eval_assignment(const AssignExpr &expr) {
    Value val = evaluate(*expr.Val);
    Curr->assign(expr.Name, val);
    return val;
}