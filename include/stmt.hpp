#pragma once

#include "common.hpp"
#include "interpreter.hpp"
#include "value.hpp"
#include "expr.hpp"

struct Stmt {
    virtual ~Stmt() = default;
    virtual void exec(Interpreter &interp) const = 0;
    virtual std::string print(AstPrinter &ast_printer) const = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct ExprStmt : public Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void exec(Interpreter &interp) const override { interp.execute_expr(*this); }
    virtual std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_expr(*this); }

};

struct DispStmt : public Stmt {
    ExprPtr expr;

    DispStmt(ExprPtr expr) : expr(std::move(expr)) {}

    void exec(Interpreter &interp) const override { interp.execute_disp(*this); }
    virtual std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_disp(*this); }
};

struct LetStmt : public Stmt {
    Token Name;
    ExprPtr Initializer;

    LetStmt(Token name, ExprPtr initializer) : Name(name), Initializer(std::move(initializer)) {}

    void exec(Interpreter &interp) const override { interp.execute_let(*this); }
    virtual std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_let(*this); }
};

struct BlockStmt : public Stmt {
    std::vector<StmtPtr> Statements;

    BlockStmt(std::vector<StmtPtr> statements) : Statements(std::move(statements)) {}

    void exec(Interpreter &interp) const override { interp.execute_block(*this); }
    virtual std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_block(*this); }
};

struct IfStmt : public Stmt {
    ExprPtr condition;
    StmtPtr then_branch, else_branch;

    IfStmt(ExprPtr condition, StmtPtr then_branch, StmtPtr else_branch)
        : condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

    void exec(Interpreter &interp) const override { interp.execute_if(*this); }
    virtual std::string print(AstPrinter &ast_printer) const override { return ast_printer.print_if(*this); }
};