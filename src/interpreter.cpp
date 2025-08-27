#include "interpreter.hpp"
#include "token.hpp"
#include "return_exception.hpp"
#include "callable.hpp"

namespace NativeFunctions {
    Value clock(Interpreter&, const std::vector<Value>&) {
        return static_cast<int>(std::time(nullptr));
    }

    Value len(Interpreter&, const std::vector<Value> &args) {
        if (args[0].is_array())
            return static_cast<int>(args[0].as_array()->size());
        if (args[0].is_string())
            return static_cast<int>(args[0].as_string().size());

        return Value();
    }

    Value str(Interpreter&, const std::vector<Value> &args) {
        return args[0].to_string();
    }

    Value int_fn(Interpreter&, const std::vector<Value> &args) {
        return args[0].as_int();
    }

    Value float_fn(Interpreter&, const std::vector<Value> &args) {
        return args[0].as_float();
    }

    Value type(Interpreter&, const std::vector<Value> &args) {
        const Value &v = args[0];
        if (v.is_null())     return "null";
        if (v.is_int())      return "int";
        if (v.is_float())    return "float";
        if (v.is_bool())     return "bool";
        if (v.is_string())   return "string";
        if (v.is_callable()) return "function";
        if (v.is_array())    return "array";
        if (v.is_object())   return "object";
        if (v.is_struct())   return "type";
        if (v.is_struct_instance()) return std::string(v.as_struct_instance()->Strct->Name);
        return "unknown";
    }

    Value print(Interpreter&, const std::vector<Value> &args) {
        for (const Value& v : args) {
            std::cout << v.to_string() << " ";
        }
        
        std::cout << std::endl;
        return {};
    }

    namespace Math {
        Value pow(Interpreter&, const std::vector<Value> &args) {
            return std::pow(args[0].as_float(), args[1].as_float());
        }

        Value abs(Interpreter&, const std::vector<Value> &args) {
            return std::abs(args[0].as_float());
        }
    
        Value sqrt(Interpreter&, const std::vector<Value> &args) {
            return std::sqrt(args[0].as_float());
        }
    
        Value sin(Interpreter&, const std::vector<Value> &args) {
            return std::sin(args[0].as_float());
        }
    
        Value cos(Interpreter&, const std::vector<Value> &args) {
            return std::cos(args[0].as_float());
        }
    
        Value tan(Interpreter&, const std::vector<Value> &args) {
            return std::tan(args[0].as_float());
        }
    
        Value round(Interpreter&, const std::vector<Value> &args) {
            return std::round(args[0].as_float());
        }
    
        Value floor(Interpreter&, const std::vector<Value> &args) {
            return std::floor(args[0].as_float());
        }
    
        Value ceil(Interpreter&, const std::vector<Value> &args) {
            return std::ceil(args[0].as_float());
        }
    
        Value min(Interpreter&, const std::vector<Value> &args) {
            return std::min(args[0].as_float(), args[1].as_float());
        }
    
        Value max(Interpreter&, const std::vector<Value> &args) {
            return std::max(args[0].as_float(), args[1].as_float());
        }
    
        Value rand(Interpreter&, const std::vector<Value>&) {
            return static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        }
    
        Value randint(Interpreter&, const std::vector<Value> &args) {
            int min = args[0].as_int();
            int max = args[1].as_int();
            return min + std::rand() % (max - min + 1);
        }
    
        Value asin(Interpreter&, const std::vector<Value> &args) {
            return std::asin(args[0].as_float());
        }
    
        Value acos(Interpreter&, const std::vector<Value> &args) {
            return std::acos(args[0].as_float());
        }
    
        Value atan(Interpreter&, const std::vector<Value> &args) {
            return std::atan(args[0].as_float());
        }
    
        Value log10(Interpreter&, const std::vector<Value> &args) {
            return std::log10(args[0].as_float());
        }
    
        Value ln(Interpreter&, const std::vector<Value> &args) {
            return std::log(args[0].as_float());
        }

        Value exp(Interpreter&, const std::vector<Value> &args) {
            return std::exp(args[0].as_float());
        }
    }

    namespace array {
        Value foreach(Interpreter &interp, const std::vector<Value> &args) {
            if (!args[0].is_array() || !args[1].is_callable()) {
                throw std::runtime_error("First argument must be an array and second must be a callable.");
            }

            const auto &arr = args[0].as_array();
            auto callable = args[1].as_callable();

            if (callable->Arity > 1) {
                for (int i = 0; i < arr->size(); i++) {
                    interp.call_function(callable, {(*arr)[i], i});
                }
            } else {
                for (const auto &item : *arr) {
                    interp.call_function(callable, {item});
                }
            }
                
            return {};
        }

        Value map(Interpreter &interp, const std::vector<Value> &args) {
            if (!args[0].is_array() || !args[1].is_callable()) {
                throw std::runtime_error("First argument must be an array and second must be a callable.");
            }

            const auto &arr = args[0].as_array();
            auto callable = args[1].as_callable();
            
            std::vector<Value> result;

            if (callable->Arity > 1) {
                for (int i = 0; i < arr->size(); i++) {
                    result.push_back(interp.call_function(callable, {(*arr)[i], i}));
                }
            } else {
                for (const auto &item : *arr) {
                    result.push_back(interp.call_function(callable, {item}));
                }
            }

            return std::make_shared<Array>(result);
        }

        Value filter(Interpreter &interp, const std::vector<Value> &args) {
            if (!args[0].is_array() || !args[1].is_callable()) {
                throw std::runtime_error("First argument must be an array and second must be a callable.");
            }

            const auto &arr = args[0].as_array();
            auto callable = args[1].as_callable();

            std::vector<Value> result;

            if (callable->Arity > 1) {
                for (int i = 0; i < arr->size(); i++) {
                    if (interp.call_function(callable, {(*arr)[i], i})) {
                        result.push_back((*arr)[i]);
                    }
                }
            } else {
                for (const auto &item : *arr) {
                    if (interp.call_function(callable, {item})) {
                        result.push_back(item);
                    }
                }
            }

            return std::make_shared<Array>(result);
        }

        Value reduce(Interpreter &interp, const std::vector<Value> &args) {
            if (!args[0].is_array() || !args[1].is_callable() || args[2].is_null()) {
                throw std::runtime_error("First argument must be an array, second must be a callable, third can't be null.");
            }

            const auto &arr = args[0].as_array();
            auto callable = args[1].as_callable();
            auto accumulator = args[2];

            if (callable->Arity > 2) {
                for (int i = 0; i < arr->size(); i++) {
                    accumulator = interp.call_function(callable, {accumulator, (*arr)[i], i});
                }
            } else {
                for (const auto &item : *arr) {
                    accumulator = interp.call_function(callable, {accumulator, item});
                }
            }
            
            return accumulator;
        }
    }
}

#define REGISTER_FUNCTION(name, arity, fn) \
    Globals->define(name, std::make_shared<Callable>(arity, fn))

#define REGISTER_ARRAY_METHOD(name, arity, fn) \
    ArrayMethods.insert({ name, std::make_shared<Callable>(arity, fn) });

Interpreter::Interpreter() {
    REGISTER_FUNCTION("clock", 0, NativeFunctions::clock);
    REGISTER_FUNCTION("len",   1, NativeFunctions::len);
    REGISTER_FUNCTION("str",   1, NativeFunctions::str);
    REGISTER_FUNCTION("int",   1, NativeFunctions::int_fn);
    REGISTER_FUNCTION("float", 1, NativeFunctions::float_fn);
    REGISTER_FUNCTION("type",  1, NativeFunctions::type);
    
    REGISTER_FUNCTION("pow",   2, NativeFunctions::Math::pow);
    REGISTER_FUNCTION("abs",   1, NativeFunctions::Math::abs);
    REGISTER_FUNCTION("sqrt",  1, NativeFunctions::Math::sqrt);
    REGISTER_FUNCTION("sin",   1, NativeFunctions::Math::sin);
    REGISTER_FUNCTION("cos",   1, NativeFunctions::Math::cos);
    REGISTER_FUNCTION("tan",   1, NativeFunctions::Math::tan);
    REGISTER_FUNCTION("round", 1, NativeFunctions::Math::round);
    REGISTER_FUNCTION("floor", 1, NativeFunctions::Math::floor);
    REGISTER_FUNCTION("ceil",  1, NativeFunctions::Math::ceil);
    REGISTER_FUNCTION("min",   2, NativeFunctions::Math::min);
    REGISTER_FUNCTION("max",   2, NativeFunctions::Math::max);

    REGISTER_FUNCTION("rand",    0, NativeFunctions::Math::rand);
    REGISTER_FUNCTION("randint", 2, NativeFunctions::Math::randint);
    REGISTER_FUNCTION("asin",    1, NativeFunctions::Math::asin);
    REGISTER_FUNCTION("acos",    1, NativeFunctions::Math::acos);
    REGISTER_FUNCTION("atan",    1, NativeFunctions::Math::atan);
    REGISTER_FUNCTION("log10",   1, NativeFunctions::Math::log10);
    REGISTER_FUNCTION("ln",      1, NativeFunctions::Math::ln);
    REGISTER_FUNCTION("exp",     1, NativeFunctions::Math::exp);

    Globals->define("pi", M_PI);

    // REGISTER_FUNCTION("array_foreach", 2, NativeFunctions::array::foreach);
    // REGISTER_FUNCTION("array_map",     2, NativeFunctions::array::map);
    // REGISTER_FUNCTION("array_filter",  2, NativeFunctions::array::filter);
    // REGISTER_FUNCTION("array_reduce",  3, NativeFunctions::array::reduce);

    REGISTER_ARRAY_METHOD("map",     1, NativeFunctions::array::map);
    REGISTER_ARRAY_METHOD("filter",  1, NativeFunctions::array::filter);
    REGISTER_ARRAY_METHOD("foreach", 1, NativeFunctions::array::foreach);
    REGISTER_ARRAY_METHOD("reduce",  2, NativeFunctions::array::reduce);
}

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
    std::visit(Overloaded{
        [&](const ExprStmt &s)     { execute_expr(s);     },
        [&](const DispStmt &s)     { execute_disp(s);     },
        [&](const LetStmt &s)      { execute_let(s);      },
        [&](const BlockStmt &s)    { execute_block(s);    },
        [&](const IfStmt &s)       { execute_if(s);       },
        [&](const WhileStmt &s)    { execute_while(s);    },
        [&](const FunctionStmt &s) { execute_function(s); },
        [&](const ReturnStmt &s)   { execute_return(s);   },
        [&](const StructStmt &s)   { execute_struct(s);   },
    }, stmt);
}

void Interpreter::execute_expr(const ExprStmt &stmt) {
    evaluate(*stmt.Expr);
}

void Interpreter::execute_disp(const DispStmt &stmt) {
    auto value = evaluate(*stmt.Expr);
    std::cout << value.to_string() << "\n";
}

void Interpreter::execute_let(const LetStmt &stmt) {
    Value initializer;
    if (stmt.Initializer) {
        initializer = evaluate(*stmt.Initializer);
    }

    Curr->define(stmt.Name.Value, initializer);
}

void Interpreter::execute_function(const FunctionStmt &stmt) {
    std::vector<Stmt*> body;
    body.reserve(stmt.Body.size());

    for (const auto &stmt : stmt.Body) {
        body.push_back(stmt.get());
    }

    Curr->define(stmt.Name.Value, std::make_shared<Callable>(stmt.Name.Value, stmt.Params, body, Curr));
}

void Interpreter::execute_block(const BlockStmt &stmt) {
    auto new_env = std::make_shared<Environment>(Curr.get());
    
    EnterEnvironmentGuard guard(*this, new_env);

    for (const auto &s : stmt.Statements) {
        execute(*s);
    }
}

void Interpreter::execute_if(const IfStmt &stmt) {
    if (evaluate(*stmt.Condition)) {
        execute(*stmt.ThenBranch);
    } else if (stmt.ElseBranch) {
        execute(*stmt.ElseBranch);
    }
}

void Interpreter::execute_while(const WhileStmt &stmt) {
    while (evaluate(*stmt.Condition)) {
        execute(*stmt.Body);
    }
}

void Interpreter::execute_return(const ReturnStmt &stmt) {
    Value value;
    if (stmt.Value) {
        value = evaluate(*stmt.Value);
    }

    throw ReturnException(value);
}

void Interpreter::execute_struct(const StructStmt &stmt) {
    Curr->define(stmt.Name.Value, {});

    std::unordered_map<std::string, Value> methods;
    methods.reserve(stmt.Methods.size());

    for (const auto &m : stmt.Methods) {
        const auto &method = std::get<FunctionStmt>(*m);

        std::vector<Stmt*> body;
        body.reserve(method.Body.size());

        for (const auto &s : method.Body) {
            body.push_back(s.get());
        }

        methods[std::string(method.Name.Value)] = std::make_shared<Callable>(method.Name.Value, method.Params, body, Curr);
    }

    auto strct = std::make_shared<Struct>(stmt.Name.Value, methods);
    Curr->assign(stmt.Name, strct);
}

Value Interpreter::evaluate(const Expr &expr) {
    return std::visit(Overloaded{
        [&](const BinaryExpr &e)   { return eval_binary(e);     },
        [&](const LogicalExpr &e)  { return eval_logical(e);    },
        [&](const UnaryExpr &e)    { return eval_unary(e);      },
        [&](const PostfixExpr &e)  { return eval_postfix(e);    },
        [&](const GroupingExpr &e) { return eval_grouping(e);   },
        [&](const LiteralExpr &e)  { return eval_literal(e);    },
        [&](const VariableExpr &e) { return eval_variable(e);   },
        [&](const AssignExpr &e)   { return eval_assignment(e); },
        [&](const CallExpr &e)     { return eval_call(e);       },
        [&](const ArrayExpr &e)    { return eval_array(e);      },
        [&](const ObjectExpr &e)   { return eval_object(e);     },
        [&](const IndexExpr &e)    { return eval_index(e);      },
        [&](const DotExpr &e)      { return eval_dot(e);        },
        [&](const TernaryExpr &e)  { return eval_ternary(e);    },
        [&](const LambdaExpr &e)   { return eval_lambda(e);     },
        [&](const SelfExpr &e)     { return eval_self(e);       },
    }, expr);
}

Value Interpreter::eval_binary(const BinaryExpr &expr) {
    auto left = evaluate(*expr.Left);
    auto right = evaluate(*expr.Right);

    switch (expr.Op.Type) {
        case TokenType::Plus:          return left +  right;
        case TokenType::Minus:         return left -  right;
        case TokenType::Mult:          return left *  right;
        case TokenType::Div:           return left /  right;
        case TokenType::Mod:           return left %  right;
        case TokenType::Greater:       return left >  right;
        case TokenType::GreaterEqual:  return left >= right;
        case TokenType::Less:          return left <  right;
        case TokenType::LessEqual:     return left <= right;
        case TokenType::Equal:         return left == right;
        case TokenType::NotEqual:      return left != right;
        case TokenType::BitOr:         return left |  right;
        case TokenType::BitXor:        return left ^  right;
        case TokenType::BitAnd:        return left &  right;
        case TokenType::BitShiftLeft:  return left << right;
        case TokenType::BitShiftRight: return left >> right;
    }

    throw std::runtime_error("Unknown binary operator: " + std::string(expr.Op.Value));
}

Value Interpreter::eval_logical(const LogicalExpr &expr) {
    auto left = evaluate(*expr.Left);
    
    if (expr.Op.Type == TokenType::Or  && left.is_truthy())  return true;
    if (expr.Op.Type == TokenType::And && !left.is_truthy()) return false;

    return evaluate(*expr.Right);
}

Value Interpreter::eval_unary(const UnaryExpr &expr) {
    auto right = evaluate(*expr.Right);

    switch (expr.Op.Type) {
        case TokenType::Not:       return !right;
        case TokenType::Minus:     return -right;
        case TokenType::BitNot:    return ~right;
    }

    if (expr.Op.Type == TokenType::Increment || expr.Op.Type == TokenType::Decrement) {
        auto value = TokenType::Increment == expr.Op.Type ? right + Value(1) : right - Value(1);
        assign_to_target(expr.Right.get(), value);
        return value;
    }

    throw std::runtime_error("Unknown unary operator: " + std::string(expr.Op.Value));
}

Value Interpreter::eval_postfix(const PostfixExpr &expr) {
    auto left = evaluate(*expr.Left);
    
    Value value;
    switch (expr.Op.Type) {
        case TokenType::Increment: value = left + Value(1); break;
        case TokenType::Decrement: value = left - Value(1); break;
        default:
            throw std::runtime_error("Unknown postfix operator: " + std::string(expr.Op.Value));
    }

    assign_to_target(expr.Left.get(), value);
    return left;
}

Value Interpreter::eval_grouping(const GroupingExpr &expr) {
    return evaluate(*expr.Grouped);
}

Value Interpreter::eval_literal(const LiteralExpr& expr) {
    return expr.Literal;
}

Value Interpreter::eval_variable(const VariableExpr &expr) {
    return Curr->get(expr.Name);
}

Value Interpreter::eval_assignment(const AssignExpr &expr) {
    Value value;
    
    if (expr.Op.Type == TokenType::Assign) {
        value = evaluate(*expr.Value);
    } else {
        auto left = evaluate(*expr.Target);
        auto right = evaluate(*expr.Value);

        switch (expr.Op.Type) {
            case TokenType::PlusEqual:  value = left + right; break;
            case TokenType::MinusEqual: value = left - right; break;
            case TokenType::MultEqual:  value = left * right; break;
            case TokenType::DivEqual:   value = left / right; break;
            case TokenType::ModEqual:   value = left % right; break;
            default:
                throw std::runtime_error("Unsupported compound assignment: " + std::string(expr.Op.Value));
        }
    }

    assign_to_target(expr.Target.get(), value);
    return value;
}

void Interpreter::assign_to_target(const Expr *target_expr, const Value &val) {
    if (auto var_expr = std::get_if<VariableExpr>(target_expr)) {
        Curr->assign(var_expr->Name, val);
    } else if (auto self_expr = std::get_if<SelfExpr>(target_expr)) {
        Curr->assign(self_expr->Keyword, val);
    } else if (auto index_expr = std::get_if<IndexExpr>(target_expr)) {
        auto container = evaluate(*index_expr->Target);
        auto idx = evaluate(*index_expr->Index);
        assign_index(container, idx, val);
        assign_to_target(index_expr->Target.get(), container);
    } else if (auto dot_expr = std::get_if<DotExpr>(target_expr)) {
        auto container = evaluate(*dot_expr->Target);
        auto key = std::string(dot_expr->Key.Value);
        assign_index(container, key, val);
        assign_to_target(dot_expr->Target.get(), container);
    } else {
        throw std::runtime_error("Invalid assignment target");
    }
}

void Interpreter::assign_index(Value &container, const Value &index, const Value &val) {
    if (container.is_array() && index.is_int()) {
        container.set_index(index.as_int(), val);
    } else if ((container.is_object() || container.is_struct_instance()) && index.is_string()) {
        container.set_index(index.as_string(), val);
    } else {
        throw std::runtime_error("Target is not indexable");
    }
}

Value Interpreter::eval_call(const CallExpr &expr) {
    auto callee = evaluate(*expr.Callee);

    std::vector<Value> args;
    args.reserve(expr.Args.size());

    for (const auto &arg : expr.Args) {
        args.push_back(evaluate(*arg));
    }

    if (callee.is_callable()) return call_function(callee.as_callable(), args);
    
    if (callee.is_struct()) {
        auto inst = std::make_shared<StructInstance>(callee.as_struct());

        auto it = inst->Strct->Methods.find("init");
        if (it != inst->Strct->Methods.end()) {
            auto constructor = it->second.as_callable();
            auto bound_constructor = bind_instance(inst, constructor);
            call_function(bound_constructor, args);
        }

        return inst;
    }
    
    return {};
}

Value Interpreter::call_function(std::shared_ptr<Callable> callable, const std::vector<Value> &args) {
    if (args.size() != callable->Arity) {
        std::stringstream ss;
        ss << "Incorrect number of arguments for function " << callable->Name
           << " (expected " << callable->Arity << ", got " << args.size() << ")";
        throw std::runtime_error(ss.str());
    }

    if (callable->Fn) return callable->Fn(*this, args);

    auto env = std::make_shared<Environment>(callable->Closure.get());

    for (size_t i = 0; i < callable->Arity; i++) {
        env->define(callable->Params[i].Value, args[i]);
    }

    EnterEnvironmentGuard guard(*this, env);

    try {
        for (const auto &stmt : callable->Body) {
            execute(*stmt);
        }
    } catch (const ReturnException &e) {
        return e.Val;
    }

    return {};
}

Value Interpreter::eval_array(const ArrayExpr &expr) {
    std::vector<Value> values;
    values.reserve(expr.Elements.size());

    for (const auto &element : expr.Elements) {
        values.push_back(evaluate(*element));
    }

    return std::make_shared<Array>(values);
}

Value Interpreter::eval_object(const ObjectExpr &expr) {
    std::unordered_map<std::string, Value> items;
    items.reserve(expr.Items.size());

    for (const auto &[key, value] : expr.Items) {
        items[key] = evaluate(*value);
    }

    return std::make_shared<Object>(std::move(items));
}

Value Interpreter::eval_index(const IndexExpr &expr) {
    auto target = evaluate(*expr.Target);
    auto index = evaluate(*expr.Index);

    if (target.is_array()) {
        if (!index.is_int()) {
            throw std::runtime_error("Array index must be an integer.");
        }

        const auto &arr = target.as_array();
        int i = index.as_int();

        if (i < 0 || i >= static_cast<int>(arr->size())) {
            throw std::runtime_error("Index out of bounds.");
        }

        return (*arr)[i];
    }

    if (target.is_object()) {
        if (!index.is_string()) {
            throw std::runtime_error("Object keys must be strings.");
        }

        const auto &obj = target.as_object();
        const auto &key = index.as_string();

        auto it = obj->find(key);
        if (it == obj->end()) {
            throw std::runtime_error("Key '" + key + "' not found in object.");
        }

        return it->second;
    }

    throw std::runtime_error("Can only index arrays or objects.");
}

Value Interpreter::eval_dot(const DotExpr &expr) {
    auto target = evaluate(*expr.Target);

    if (target.is_struct_instance()) {
        auto inst = target.as_struct_instance();
        auto property = inst->get(std::string(expr.Key.Value));

        if (property.is_callable()) {
            auto method = property.as_callable();
            return bind_instance(inst, method);
        }

        return property;
    }

    if (target.is_object()) {
        auto obj = target.as_object();
        const auto &key = std::string(expr.Key.Value);

        auto it = obj->find(key);
        if (it == obj->end()) {
            throw std::runtime_error("Key '" + key + "' not found in object.");
        }

        return it->second;
    }

    if (target.is_array()) {
        auto arr = target.as_array();
        const auto &key = expr.Key.Value;
    
        auto it = ArrayMethods.find(key);
        if (it != ArrayMethods.end()) {
            auto callable = it->second.as_callable();
            return std::make_shared<Callable>(callable->Arity, [arr, callable](Interpreter &interp, const std::vector<Value> &args) -> Value {
                std::vector<Value> call_args;
                call_args.reserve(args.size() + 1);
                call_args.push_back(Value(arr));
                call_args.insert(call_args.end(), args.begin(), args.end());
                return callable->Fn(interp, call_args);
            });
        }
    
        throw std::runtime_error("Array has no method '" + std::string(key) + "'");
    }

    throw std::runtime_error("Dot notation can only be used with objects.");
}

Value Interpreter::eval_ternary(const TernaryExpr &expr) {
    if (evaluate(*expr.Condition)) {
        return evaluate(*expr.Left);
    }

    return evaluate(*expr.Right);
}

Value Interpreter::eval_lambda(const LambdaExpr &expr) {
    std::vector<Stmt*> body;
    body.reserve(expr.Body.size());

    for (const StmtPtr &stmt : expr.Body) {
        body.push_back(stmt.get());
    }

    return std::make_shared<Callable>("_", expr.Params, body, Curr);
}

Value Interpreter::eval_self(const SelfExpr &expr) {
    return Curr->get(expr.Keyword);
}

std::shared_ptr<Callable> Interpreter::bind_instance(std::shared_ptr<StructInstance> inst, std::shared_ptr<Callable> method) {
    auto env = std::make_shared<Environment>(method->Closure.get());
    env->define("self", inst);

    return std::make_shared<Callable>(
        method->Name,
        method->Params,
        method->Body,
        env
    );
}