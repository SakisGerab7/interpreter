#include "ast_printer.hpp"

namespace Color {
    constexpr const char *Reset   = "\033[0m";
    constexpr const char *Bold    = "\033[1m";

    constexpr const char *Keyword = "\033[1;35m"; // Magenta
    constexpr const char *String  = "\033[0;32m"; // Green
    constexpr const char *Number  = "\033[1;36m"; // Cyan
    constexpr const char *Ident   = "\033[1;34m"; // Blue
    constexpr const char *Op      = "\033[0;33m"; // Yellow
}

inline std::string colored(std::string_view text, const char *color) {
    return std::string(color) + std::string(text) + Color::Reset;
}

std::string AstPrinter::indent() const {
    return std::string(indent_level * 2, ' ');
}

std::string AstPrinter::print(const Expr &expr) {
    return std::visit(Overloaded{
        [&](const BinaryExpr &e)   { return print_binary(e);    },
        [&](const LogicalExpr &e)  { return print_logical(e);   },
        [&](const UnaryExpr &e)    { return print_unary(e);     },
        [&](const PostfixExpr &e)  { return print_postfix(e);   },
        [&](const GroupingExpr &e) { return print_grouping(e);  },
        [&](const LiteralExpr &e)  { return print_literal(e);   },
        [&](const VariableExpr &e) { return print_variable(e);  },
        [&](const AssignExpr &e)   { return print_assign(e);    },
        [&](const SetDotExpr &e)   { return print_set_dot(e);   },
        [&](const SetIndexExpr &e) { return print_set_index(e); },
        [&](const CallExpr &e)     { return print_call(e);      },
        [&](const ArrayExpr &e)    { return print_array(e);     },
        [&](const ObjectExpr &e)   { return print_object(e);    },
        [&](const IndexExpr &e)    { return print_index(e);     },
        [&](const DotExpr &e)      { return print_dot(e);       },
        [&](const TernaryExpr &e)  { return print_ternary(e);   },
        [&](const LambdaExpr &e)   { return print_lambda(e);    },
        [&](const SelfExpr &e)     { return print_self(e);      },
        [&](const SpawnExpr &e)    { return print_spawn(e);     },
    }, expr);
}

std::string AstPrinter::print(const Stmt &stmt) {
    return std::visit(Overloaded{
        [&](const ExprStmt &s)     { return print_expr(s);     },
        [&](const DispStmt &s)     { return print_disp(s);     },
        [&](const LetStmt &s)      { return print_let(s);      },
        [&](const BlockStmt &s)    { return print_block(s);    },
        [&](const IfStmt &s)       { return print_if(s);       },
        [&](const WhileStmt &s)    { return print_while(s);    },
        [&](const FunctionStmt &s) { return print_function(s); },
        [&](const ReturnStmt &s)   { return print_return(s);   },
        [&](const StructStmt &s)   { return print_struct(s);   },
    }, stmt);
}

std::string AstPrinter::print_binary(const BinaryExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op), expr.left.get(), expr.right.get());
}

std::string AstPrinter::print_logical(const LogicalExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op), expr.left.get(), expr.right.get());
}

std::string AstPrinter::print_unary(const UnaryExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op), expr.right.get());
}

std::string AstPrinter::print_postfix(const PostfixExpr &expr) {
    return parenthesize(colored("postfix ", Color::Keyword) + colored(expr.op.value, Color::Op), expr.left.get());
}

std::string AstPrinter::print_grouping(const GroupingExpr &expr) {
    return parenthesize(colored("group", Color::Keyword), expr.grouped.get());
}

std::string AstPrinter::print_literal(const LiteralExpr &expr) {
    return colored(expr.literal.to_string(), expr.literal.is_string() ? Color::String : Color::Number);
}

std::string AstPrinter::print_variable(const VariableExpr &expr) {
    return colored(expr.name.value, Color::Ident);
}

std::string AstPrinter::print_assign(const AssignExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op) + " " + colored(expr.name.value, Color::Ident), expr.value.get());
}

std::string AstPrinter::print_set_dot(const SetDotExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op) + " " + colored(expr.key.value, Color::String), expr.target.get(), expr.value.get());
}

std::string AstPrinter::print_set_index(const SetIndexExpr &expr) {
    return parenthesize(colored(expr.op.value, Color::Op), expr.index.get(), expr.target.get(), expr.value.get());
}

std::string AstPrinter::print_call(const CallExpr &expr) {
    std::vector<const Expr*> args;
    args.reserve(expr.args.size() + 1);

    args.push_back(expr.callee.get());
    for (const auto &arg : expr.args) {
        args.push_back(arg.get());
    }

    return parenthesize(colored("call", Color::Keyword), args);
}

std::string AstPrinter::print_array(const ArrayExpr &expr) {
    std::vector<const Expr*> elements;
    elements.reserve(expr.elements.size());

    for (const auto &element : expr.elements) {
        elements.push_back(element.get());
    }

    return parenthesize(colored("array", Color::Keyword), elements);
}

std::string AstPrinter::print_object(const ObjectExpr &expr) {
    std::stringstream keys;
    std::vector<const Expr*> values;
    values.reserve(expr.items.size());

    for (const auto &[key, value] : expr.items) {
        keys << " " << key;
        values.push_back(value.get());
    }

    return parenthesize(colored("object", Color::Keyword) + colored(keys.str(), Color::String), values);
}

std::string AstPrinter::print_index(const IndexExpr &expr) {
    return parenthesize(colored("[]", Color::Op), expr.target.get(), expr.index.get());
}

std::string AstPrinter::print_dot(const DotExpr &expr) {
    return parenthesize(colored(". ", Color::Op) + colored(expr.key.value, Color::Ident), expr.target.get());
}

std::string AstPrinter::print_ternary(const TernaryExpr &expr) {
    return parenthesize(colored("?:", Color::Op), expr.condition.get(), expr.left.get(), expr.right.get());
}

std::string AstPrinter::print_lambda(const LambdaExpr &expr) {
    std::stringstream out;
    out << "(" << colored("fn", Color::Keyword) << " (";

    for (size_t i = 0; i < expr.params.size(); i++) {
        out << colored(expr.params[i].value, Color::Ident);
        if (i < expr.params.size() - 1) {
            out << " ";
        }
    }

    out << ")";

    if (!expr.body->empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : *expr.body) {
            out << "\n" << print(*s);
        }
    }

    out << ")";

    return out.str();
}

std::string AstPrinter::print_self(const SelfExpr &expr) {
    return colored(expr.keyword.value, Color::Keyword);
}

std::string AstPrinter::print_spawn(const SpawnExpr &expr) {
    std::stringstream out;
    out << "(" << colored("spawn", Color::Keyword) << " " << (expr.count ? print(*expr.count) : "");

    if (!expr.statements->empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : *expr.statements) {
            out << "\n" << print(*s);
        }
    }

    return out.str();
}

std::string AstPrinter::print_expr(const ExprStmt &stmt) {
    return indent() + parenthesize(colored("expr", Color::Keyword), stmt.expr.get());
}

std::string AstPrinter::print_disp(const DispStmt &stmt) {
    return indent() + parenthesize(colored("disp", Color::Keyword), stmt.expr.get());
}

std::string AstPrinter::print_let(const LetStmt &stmt) {
    return indent() + parenthesize(colored("let ", Color::Keyword) + colored(stmt.name.value, Color::Ident), stmt.initializer.get());
}

std::string AstPrinter::print_function(const FunctionStmt &stmt) {
    std::stringstream out;

    out << indent() << "(" << colored("fn ", Color::Keyword) << colored(stmt.name.value, Color::Ident) << "(";

    for (size_t i = 0; i < stmt.params.size(); i++) {
        out << colored(stmt.params[i].value, Color::Ident);
        if (i < stmt.params.size() - 1) {
            out << " ";
        }
    }

    out << ")";

    if (!stmt.body->empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : *stmt.body) {
            out << "\n" << print(*s);
        }
    }

    out << ")";

    return out.str();
}

std::string AstPrinter::print_block(const BlockStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("block", Color::Keyword);

    if (!stmt.statements->empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : *stmt.statements) {
            out << "\n" << print(*s);
        }
    }

    out << ")";
    return out.str();
}

std::string AstPrinter::print_if(const IfStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("if", Color::Keyword) << " " << print(*stmt.condition) << "\n";

    {
        IndentGuard guard(indent_level);
        out << print(*stmt.then_branch);
        if (stmt.else_branch) {
            out << "\n" << indent() << colored("else", Color::Keyword) << "\n";
            out << print(*stmt.else_branch) << ")";
        } else {
            out << ")";
        }
    }

    return out.str();
}

std::string AstPrinter::print_while(const WhileStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("while", Color::Keyword) << " " << print(*stmt.condition) << "\n";

    {
        IndentGuard guard(indent_level);
        out << print(*stmt.body) << ")";
    }

    return out.str();
}

std::string AstPrinter::print_return(const ReturnStmt &stmt) {
    return indent() + parenthesize(colored("return", Color::Keyword), stmt.value.get());
}

std::string AstPrinter::print_struct(const StructStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("struct", Color::Keyword);

    if (!stmt.methods.empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : stmt.methods) {
            out << "\n" << print(*s);
        }
    }

    return out.str();
}

template<typename... Args>
std::string AstPrinter::parenthesize(const std::string &name, const Args... args) {
    std::stringstream out;
    out << "(" << name;
    ((args ? (out << " " << print(*args)) : (out << "")), ...);
    out << ")";
    return out.str();
}

std::string AstPrinter::parenthesize(const std::string &name, const std::vector<const Expr*> &exprs) {
    std::stringstream out;
    out << "(" << name;
    for (const Expr *expr : exprs) {
        if (expr) out << " " << print(*expr);
    }

    out << ")";
    return out.str();
}
