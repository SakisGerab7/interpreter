#include "interpreter.hpp"
#include "expr.hpp"
#include "token.hpp"
#include "stmt.hpp"
#include "callable.hpp"

namespace NativeFunctions {

    Value clock(Interpreter&, const std::vector<Value>&) {
        return static_cast<int>(std::time(nullptr));
    }

    Value len(Interpreter&, const std::vector<Value> &args) {
        if (args[0].is_array())
            return static_cast<int>(args[0].as_array().size());
        if (args[0].is_string())
            return static_cast<int>(args[0].as_string().size());

        return Value();
    }

    Value str(Interpreter&, const std::vector<Value>& args) {
        return args[0].to_string();
    }

    Value int_fn(Interpreter&, const std::vector<Value>& args) {
        return args[0].as_int();
    }

    Value float_fn(Interpreter&, const std::vector<Value>& args) {
        return args[0].as_float();
    }

    Value type(Interpreter&, const std::vector<Value>& args) {
        const Value &v = args[0];
        if (v.is_int()) return "int";
        if (v.is_float()) return "float";
        if (v.is_bool()) return "bool";
        if (v.is_string()) return "string";
        if (v.is_null()) return "null";
        if (v.is_callable()) return "function";
        return "unknown";
    }

    Value print(Interpreter&, const std::vector<Value>& args) {
        for (const Value& v : args) {
            std::cout << v.to_string() << " ";
        }
        std::cout << std::endl;
        return {};
    }

    namespace Math {
        Value pow(Interpreter&, const std::vector<Value>& args) {
            return std::pow(args[0].as_float(), args[1].as_float());
        }

        Value abs(Interpreter&, const std::vector<Value>& args) {
            return std::abs(args[0].as_float());
        }
    
        Value sqrt(Interpreter&, const std::vector<Value>& args) {
            return std::sqrt(args[0].as_float());
        }
    
        Value sin(Interpreter&, const std::vector<Value>& args) {
            return std::sin(args[0].as_float());
        }
    
        Value cos(Interpreter&, const std::vector<Value>& args) {
            return std::cos(args[0].as_float());
        }
    
        Value tan(Interpreter&, const std::vector<Value>& args) {
            return std::tan(args[0].as_float());
        }
    
        Value round(Interpreter&, const std::vector<Value>& args) {
            return std::round(args[0].as_float());
        }
    
        Value floor(Interpreter&, const std::vector<Value>& args) {
            return std::floor(args[0].as_float());
        }
    
        Value ceil(Interpreter&, const std::vector<Value>& args) {
            return std::ceil(args[0].as_float());
        }
    
        Value min(Interpreter&, const std::vector<Value>& args) {
            return std::min(args[0].as_float(), args[1].as_float());
        }
    
        Value max(Interpreter&, const std::vector<Value>& args) {
            return std::max(args[0].as_float(), args[1].as_float());
        }
    
        Value rand(Interpreter&, const std::vector<Value>&) {
            return static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        }
    
        Value randint(Interpreter&, const std::vector<Value>& args) {
            int min = args[0].as_int();
            int max = args[1].as_int();
            return min + std::rand() % (max - min + 1);
        }
    
        Value asin(Interpreter&, const std::vector<Value>& args) {
            return std::asin(args[0].as_float());
        }
    
        Value acos(Interpreter&, const std::vector<Value>& args) {
            return std::acos(args[0].as_float());
        }
    
        Value atan(Interpreter&, const std::vector<Value>& args) {
            return std::atan(args[0].as_float());
        }
    
        Value log10(Interpreter&, const std::vector<Value>& args) {
            return std::log10(args[0].as_float());
        }
    
        Value ln(Interpreter&, const std::vector<Value>& args) {
            return std::log(args[0].as_float());
        }

        Value exp(Interpreter&, const std::vector<Value>& args) {
            return std::exp(args[0].as_float());
        }
    }
}


Interpreter::Interpreter() {
    Globals->define("number_four", std::make_shared<Callable>(0, [](Interpreter &, const std::vector<Value> &) {
        return 4;
    }));

    Globals->define("clock", std::make_shared<Callable>(0, NativeFunctions::clock));
    Globals->define("len",   std::make_shared<Callable>(1, NativeFunctions::len));
    Globals->define("str",   std::make_shared<Callable>(1, NativeFunctions::str));
    Globals->define("int",   std::make_shared<Callable>(1, NativeFunctions::int_fn));
    Globals->define("float", std::make_shared<Callable>(1, NativeFunctions::float_fn));
    Globals->define("type",  std::make_shared<Callable>(1, NativeFunctions::type));
    Globals->define("print", std::make_shared<Callable>(-1, NativeFunctions::print)); // Use -1 for variadic
    
    Globals->define("pow",   std::make_shared<Callable>(2, NativeFunctions::Math::pow));
    Globals->define("abs",   std::make_shared<Callable>(1, NativeFunctions::Math::abs));
    Globals->define("sqrt",  std::make_shared<Callable>(1, NativeFunctions::Math::sqrt));
    Globals->define("sin",   std::make_shared<Callable>(1, NativeFunctions::Math::sin));
    Globals->define("cos",   std::make_shared<Callable>(1, NativeFunctions::Math::cos));
    Globals->define("tan",   std::make_shared<Callable>(1, NativeFunctions::Math::tan));
    Globals->define("round", std::make_shared<Callable>(1, NativeFunctions::Math::round));
    Globals->define("floor", std::make_shared<Callable>(1, NativeFunctions::Math::floor));
    Globals->define("ceil",  std::make_shared<Callable>(1, NativeFunctions::Math::ceil));
    Globals->define("min",   std::make_shared<Callable>(2, NativeFunctions::Math::min));
    Globals->define("max",   std::make_shared<Callable>(2, NativeFunctions::Math::max));

    Globals->define("rand",    std::make_shared<Callable>(0, NativeFunctions::Math::rand));
    Globals->define("randint", std::make_shared<Callable>(2, NativeFunctions::Math::randint));
    Globals->define("asin",    std::make_shared<Callable>(1, NativeFunctions::Math::asin));
    Globals->define("acos",    std::make_shared<Callable>(1, NativeFunctions::Math::acos));
    Globals->define("atan",    std::make_shared<Callable>(1, NativeFunctions::Math::atan));
    Globals->define("log10",   std::make_shared<Callable>(1, NativeFunctions::Math::log10));
    Globals->define("ln",      std::make_shared<Callable>(1, NativeFunctions::Math::ln));
    Globals->define("exp",     std::make_shared<Callable>(1, NativeFunctions::Math::exp));

    Globals->define("pi", M_PI);


}

void Interpreter::interpret(const std::vector<StmtPtr> &statements)
{
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

void Interpreter::execute_function(const FunctionStmt &stmt) {
    Curr->define(stmt.Name.Value, std::make_shared<Callable>(&stmt));
}

void Interpreter::execute_block(const BlockStmt &stmt, std::shared_ptr<Environment> new_env) {
    std::shared_ptr<Environment> prev = Curr;
    
    try {
        Curr = new_env;

        for (const StmtPtr &s : stmt.Statements) {
            execute(*s);
        }
        
        Curr = prev;
    } catch (...) {
        Curr = prev;
        throw;
    }
}

void Interpreter::execute_block(const BlockStmt &stmt) {
    execute_block(stmt, std::make_shared<Environment>(Curr.get()));
}

void Interpreter::execute_if(const IfStmt &stmt) {
    if (evaluate(*stmt.Condition).is_truthy()) {
        execute(*stmt.then_branch);
    } else if (stmt.else_branch) {
        execute(*stmt.else_branch);
    }
}

void Interpreter::execute_while(const WhileStmt &stmt) {
    while (evaluate(*stmt.Condition).is_truthy()) {
        execute(*stmt.Body);
    }
}

Value Interpreter::evaluate(const Expr &expr) {
    return expr.eval(*this);
}

Value Interpreter::eval_binary(const BinaryExpr &expr) {
    Value left = evaluate(*expr.Left);
    Value right = evaluate(*expr.Right);

    switch (expr.Op.Type) {
        case TokenType::Plus:         return left +  right;
        case TokenType::Minus:        return left -  right;
        case TokenType::Mult:         return left *  right;
        case TokenType::Div:          return left /  right;
        case TokenType::Mod:          return left %  right;
        case TokenType::Greater:      return left >  right;
        case TokenType::GreaterEqual: return left >= right;
        case TokenType::Less:         return left <  right;
        case TokenType::LessEqual:    return left <= right;
        case TokenType::Equal:        return left == right;
        case TokenType::NotEqual:     return left != right;
    }

    throw std::runtime_error("Unknown binary operator: " + expr.Op.Value);
}

Value Interpreter::eval_logical(const LogicalExpr &expr) {
    Value left = evaluate(*expr.Left);
    
    if (expr.Op.Type == TokenType::Or  && left.is_truthy())  return true;
    if (expr.Op.Type == TokenType::And && !left.is_truthy()) return false;

    return evaluate(*expr.Right);
}

Value Interpreter::eval_unary(const UnaryExpr &expr) {
    Value right = evaluate(*expr.Right);

    switch (expr.Op.Type) {
        case TokenType::Not:   return logical_not(right);
        case TokenType::Minus: return -right;
    }

    throw std::runtime_error("Unknown unary operator: " + expr.Op.Value);
}

Value Interpreter::eval_variable(const VariableExpr &expr) {
    return Curr->get(expr.Name);
}

Value Interpreter::eval_assignment(const AssignExpr &expr) {
    Value val;
    
    if (expr.Op.Type == TokenType::Assign) {
        val = evaluate(*expr.Val);
    } else {
        Value left = Curr->get(expr.Name);
        Value right = evaluate(*expr.Val);

        switch (expr.Op.Type) {
            case TokenType::PlusEqual:  val = left + right; break;
            case TokenType::MinusEqual: val = left - right; break;
            case TokenType::MultEqual:  val = left * right; break;
            case TokenType::DivEqual:   val = left / right; break;
            case TokenType::ModEqual:   val = left % right; break;
            default:
                throw std::runtime_error("Unsupported compound assignment: " + expr.Op.Value);
        }
    }

    Curr->assign(expr.Name, val);
    return val;
}

Value Interpreter::eval_call(const CallExpr &expr) {
    Value callee = evaluate(*expr.Callee);

    std::vector<Value> args;
    for (const ExprPtr &arg : expr.Args) {
        args.push_back(evaluate(*arg));
    }

    std::shared_ptr<Callable> function = callee.as_callable();
    return function->call(*this, args);
}

Value Interpreter::eval_array(const ArrayExpr &expr) {
    std::vector<Value> values;
    for (const ExprPtr &element : expr.Elements) {
        values.push_back(evaluate(*element));
    }

    return Value(values);
}

Value Interpreter::eval_index(const IndexExpr &expr) {
    Value target = evaluate(*expr.Target);
    Value index = evaluate(*expr.Index);

    if (!target.is_array()) {
        throw std::runtime_error("Can only index arrays.");
    }

    if (!index.is_int()) {
        throw std::runtime_error("Array index must be an integer.");
    }

    const std::vector<Value> &arr = target.as_array();
    int i = index.as_int();

    if (i < 0 || i >= static_cast<int>(arr.size())) {
        throw std::runtime_error("Index out of bounds.");
    }

    return arr[i];
}
