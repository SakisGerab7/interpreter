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

//===================== AstPrinter =====================//

std::string AstPrinter::indent() const {
    return std::string(indent_level * 2, ' ');
}

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

std::string AstPrinter::print(const Expr &expr) {
    return std::visit(Overloaded{
        [&](const BinaryExpr &e)   { return print_binary(e);     },
        [&](const LogicalExpr &e)  { return print_logical(e);    },
        [&](const UnaryExpr &e)    { return print_unary(e);      },
        [&](const GroupingExpr &e) { return print_grouping(e);   },
        [&](const LiteralExpr &e)  { return print_literal(e);    },
        [&](const VariableExpr &e) { return print_variable(e);   },
        [&](const AssignExpr &e)   { return print_assignment(e); },
        [&](const CallExpr &e)     { return print_call(e);       },
        [&](const ArrayExpr &e)    { return print_array(e);      },
        [&](const ObjectExpr &e)   { return print_object(e);     },
        [&](const IndexExpr &e)    { return print_index(e);      },
        [&](const TernaryExpr &e)  { return print_ternary(e);    }
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
        [&](const ReturnStmt &s)   { return print_return(s);   }
    }, stmt);
}

//===================== Expressions =====================//

std::string AstPrinter::print_binary(const BinaryExpr &expr) {
    return parenthesize(colored(expr.Op.Value, Color::Op), expr.Left.get(), expr.Right.get());
}

std::string AstPrinter::print_logical(const LogicalExpr &expr) {
    return parenthesize(colored(expr.Op.Value, Color::Op), expr.Left.get(), expr.Right.get());
}

std::string AstPrinter::print_unary(const UnaryExpr &expr) {
    return parenthesize(colored(expr.Op.Value, Color::Op), expr.Right.get());
}

std::string AstPrinter::print_grouping(const GroupingExpr &expr) {
    return parenthesize(colored("group", Color::Keyword), expr.Grouped.get());
}

std::string AstPrinter::print_literal(const LiteralExpr &expr) {
    return colored(expr.Lit.to_string(), expr.Lit.is_string() ? Color::String : Color::Number);
}

std::string AstPrinter::print_variable(const VariableExpr &expr) {
    return colored(expr.Name.Value, Color::Ident);
}

std::string AstPrinter::print_assignment(const AssignExpr &expr) {
    return parenthesize(colored(expr.Op.Value, Color::Op), expr.Target.get(), expr.Val.get());
}

std::string AstPrinter::print_call(const CallExpr &expr) {
    std::vector<const Expr*> args;
    args.push_back(expr.Callee.get());
    for (const auto &arg : expr.Args) {
        args.push_back(arg.get());
    }

    return parenthesize(colored("call", Color::Keyword), args);
}

std::string AstPrinter::print_array(const ArrayExpr &expr) {
    std::vector<const Expr*> elements;
    for (const auto &element : expr.Elements) {
        elements.push_back(element.get());
    }

    return parenthesize(colored("array", Color::Keyword), elements);
}

std::string AstPrinter::print_object(const ObjectExpr &expr) {
    std::stringstream keys;
    std::vector<const Expr*> values;
    for (const auto &[key, value] : expr.Items) {
        keys << " " << key;
        values.push_back(value.get());
    }

    return parenthesize(colored("object", Color::Keyword) + colored(keys.str(), Color::String), values);
}

std::string AstPrinter::print_index(const IndexExpr &expr) {
    return parenthesize(colored("[]", Color::Op), expr.Target.get(), expr.Index.get());
}

std::string AstPrinter::print_ternary(const TernaryExpr &expr) {
    return parenthesize(colored("?:", Color::Op), expr.Condition.get(), expr.Left.get(), expr.Right.get());
}

//===================== Statements =====================//

std::string AstPrinter::print_expr(const ExprStmt &stmt) {
    return indent() + parenthesize(colored("expr", Color::Keyword), stmt.expr.get());
}

std::string AstPrinter::print_disp(const DispStmt &stmt) {
    return indent() + parenthesize(colored("disp", Color::Keyword), stmt.expr.get());
}

std::string AstPrinter::print_let(const LetStmt &stmt) {
    return indent() + parenthesize(colored("let ", Color::Keyword) + colored(stmt.Name.Value, Color::Ident), stmt.Initializer.get());
}

std::string AstPrinter::print_function(const FunctionStmt &stmt) {
    std::stringstream out;

    out << indent() << "(" << colored("fn ", Color::Keyword) << colored(stmt.Name.Value, Color::Ident) << "(";

    for (size_t i = 0; i < stmt.Params.size(); i++) {
        out << colored(stmt.Params[i].Value, Color::Ident) << ((i + 1 < stmt.Params.size()) ? " " : ")\n");
    }

    {
        IndentGuard guard(indent_level);
        out << print(*stmt.Body) << ")";
    }

    return out.str();
}

std::string AstPrinter::print_block(const BlockStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("block", Color::Keyword);

    if (!stmt.Statements.empty()) {
        IndentGuard guard(indent_level);

        for (const auto &s : stmt.Statements) {
            out << "\n" << print(*s);
        }
    }

    out << ")";
    return out.str();
}

std::string AstPrinter::print_if(const IfStmt &stmt) {
    std::stringstream out;
    out << indent() << "(" << colored("if", Color::Keyword) << " " << print(*stmt.Condition) << "\n";

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
    out << indent() << "(" << colored("while", Color::Keyword) << " " << print(*stmt.Condition) << "\n";

    {
        IndentGuard guard(indent_level);
        out << print(*stmt.Body) << ")";
    }

    return out.str();
}

std::string AstPrinter::print_return(const ReturnStmt &stmt) {
    return indent() + parenthesize(colored("return", Color::Keyword), stmt.Val.get());
}

//===================== Parenthesize Helpers =====================//

template<typename... Args>
std::string AstPrinter::parenthesize(const std::string &name, const Args... args) {
    std::vector<const Expr *> exprs{args...};
    return parenthesize(name, exprs);
}

std::string AstPrinter::parenthesize(const std::string &name, const std::vector<const Expr *> &exprs) {
    std::stringstream out;
    out << "(" << name;
    for (const Expr *expr : exprs) {
        if (expr) out << " " << print(*expr);
    }
    out << ")";
    return out.str();
}
