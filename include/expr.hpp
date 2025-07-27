#pragma once

#include "common.hpp"
#include "ast_printer.hpp"
#include "interpreter.hpp"
#include "value.hpp"
#include "token.hpp"

struct Expr {
    virtual ~Expr() = default;
    virtual Value eval(Interpreter &interp) const = 0;
    virtual std::string print(AstPrinter &ast_printer) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct BinaryExpr : public Expr {
    ExprPtr Left, Right;
    Token Op;

    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : Left(std::move(left)), Op(op), Right(std::move(right)) {}

    Value eval(Interpreter &interp) const override { return interp.eval_binary(*this); }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_binary(*this); }
};

struct UnaryExpr : public Expr {
    ExprPtr Right;
    Token Op;

    UnaryExpr(Token op, ExprPtr right)
        : Op(op), Right(std::move(right)) {}

    Value eval(Interpreter &interp) const override { return interp.eval_unary(*this); }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_unary(*this); }
};

struct GroupingExpr : public Expr {
    ExprPtr Grouped;

    GroupingExpr(ExprPtr grouped)
        : Grouped(std::move(grouped)) {}

    Value eval(Interpreter &interp) const override { return Grouped->eval(interp); }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_grouping(*this); }
};

struct LiteralExpr : public Expr {
    Value Lit;

    LiteralExpr(Value lit) : Lit(lit) {}

    Value eval(Interpreter &interp) const override { return Lit; }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_literal(*this); }
};

struct VariableExpr : public Expr {
    Token Name;

    VariableExpr(Token name) : Name(name) {}

    Value eval(Interpreter &interp) const override { return interp.eval_variable(*this); }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_variable(*this); }
};

struct AssignExpr : public Expr {
    Token Name;
    ExprPtr Val;

    AssignExpr(Token name, ExprPtr value)
        : Name(name), Val(std::move(value)) {}

    Value eval(Interpreter &interp) const override { return interp.eval_assignment(*this); }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_assignment(*this); }
};

struct CallExpr : public Expr {
    ExprPtr Callee;
    std::vector<ExprPtr> Args;

    CallExpr(ExprPtr callee, std::vector<ExprPtr> args)
        : Callee(std::move(callee)), Args(std::move(args)) {}

    Value eval(Interpreter &interp) const override { return {}; }
    std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_call(*this); }
};