#include "codegen.hpp"

Function::Ptr Codegen::compile(const std::shared_ptr<std::vector<StmtPtr>> &statements) {
    begin_function("main", 0);

    for (const auto &s : *statements) {
        generate(*s);
    }

    return end_function();
}

void Codegen::begin_function(const std::string &name, int arity, bool is_method) {
    auto new_func = std::make_shared<Function>(name, arity);
    auto new_scope = std::make_shared<ScopeManager>(scopes, is_method);
    function_stack.push_back(new_func);
    curr = new_func;
    scopes = new_scope;
}

Function::Ptr Codegen::end_function(bool is_init) {
    emit_return(is_init);

    auto finished_func = curr;
    finished_func->upvalue_count = static_cast<int>(scopes->upvalues.size());
    function_stack.pop_back();

    if (!function_stack.empty()) {
        curr = function_stack.back();
    }

    scopes = scopes->parent;
    return finished_func;
}

void Codegen::generate(const Expr &expr) {
    std::visit(Overloaded{
        [&](const BinaryExpr &e)    { generate_binary(e);    },
        [&](const LogicalExpr &e)   { generate_logical(e);   },
        [&](const UnaryExpr &e)     { generate_unary(e);     },
        [&](const PostfixExpr &e)   { generate_postfix(e);   },
        [&](const GroupingExpr &e)  { generate_grouping(e);  },
        [&](const LiteralExpr &e)   { generate_literal(e);   },
        [&](const VariableExpr &e)  { generate_variable(e);  },
        [&](const AssignExpr &e)    { generate_assign(e);    },
        [&](const SetDotExpr &e)    { generate_set_dot(e);   },
        [&](const SetIndexExpr &e)  { generate_set_index(e); },
        [&](const CallExpr &e)      { generate_call(e);      },
        [&](const ArrayExpr &e)     { generate_array(e);     },
        [&](const ObjectExpr &e)    { generate_object(e);    },
        [&](const IndexExpr &e)     { generate_index(e);     },
        [&](const DotExpr &e)       { generate_dot(e);       },
        [&](const TernaryExpr &e)   { generate_ternary(e);   },
        [&](const LambdaExpr &e)    { generate_lambda(e);    },
        [&](const SelfExpr &e)      { generate_self(e);      },
        [&](const SpawnExpr &e)     { generate_spawn(e);     }
    }, expr);
}

void Codegen::generate(const Stmt &stmt) {
    std::visit(Overloaded{
        [&](const ExprStmt &s)     { generate_expr(s);     },
        [&](const DispStmt &s)     { generate_disp(s);     },
        [&](const LetStmt &s)      { generate_let(s);      },
        [&](const BlockStmt &s)    { generate_block(s);    },
        [&](const IfStmt &s)       { generate_if(s);       },
        [&](const WhileStmt &s)    { generate_while(s);    },
        [&](const FunctionStmt &s) { generate_function(s); },
        [&](const ReturnStmt &s)   { generate_return(s);   },
        [&](const StructStmt &s)   { generate_struct(s);   },
    }, stmt);
}

inline void Codegen::begin_scope() {
    scopes->begin_scope();
}

inline void Codegen::end_scope() {
    scopes->end_scope([&](bool is_captured) {
        emit(is_captured ? OP_CLOSE_UPVALUE : OP_POP);
    });
}

inline uint16_t Codegen::make_constant(const Value &v) {
    return curr->chunk.add_constant(v);
}

void Codegen::emit_constant(const Value &v) {
    uint16_t idx = make_constant(v);
    emit(OP_CONST);
    emit(idx);
}

void Codegen::emit_iconst8(int8_t v) {
    emit(OP_ICONST8);
    emit(static_cast<uint8_t>(v));
}

void Codegen::emit_iconst16(int16_t v) {
    emit(OP_ICONST16);
    emit(static_cast<uint16_t>(v));
}

void Codegen::emit_return(bool is_init) {
    if (is_init) {
        emit(OP_LOAD_LOCAL);
        emit(static_cast<uint8_t>(0)); // load 'self'
    } else {
        emit(OP_NULL);
    }

    emit(OP_RETURN);
}

ScopeManager::ResolveResult Codegen::resolve_variable(const Token &name) {
    return scopes->resolve_variable(name);
}

void Codegen::emit_load_var(const Token &name) {
    auto res = resolve_variable(name);
    switch (res.type) {
        case ScopeManager::VarType::Local: {
            emit(OP_LOAD_LOCAL);
            emit(static_cast<uint8_t>(res.index));
            return;
        }
        case ScopeManager::VarType::Upvalue: {
            emit(OP_LOAD_UPVALUE);
            emit(static_cast<uint8_t>(res.index));
            return;
        }
        case ScopeManager::VarType::Global: {
            uint16_t name_idx = make_constant(name.value);
            emit(OP_LOAD_GLOBAL);
            emit(name_idx);
            return;
        }
    }
}

void Codegen::emit_store_var(const Token &name) {
    auto res = resolve_variable(name);
    switch (res.type) {
        case ScopeManager::VarType::Local: {
            emit(OP_STORE_LOCAL);
            emit(static_cast<uint8_t>(res.index));
            return;
        }
        case ScopeManager::VarType::Upvalue: {
            emit(OP_STORE_UPVALUE);
            emit(static_cast<uint8_t>(res.index));
            return;
        }
        case ScopeManager::VarType::Global: {
            uint16_t name_idx = make_constant(name.value);
            emit(OP_STORE_GLOBAL);
            emit(name_idx);
            return;
        }
    }
}

void Codegen::emit_compound_op(const Token &op) {
    switch (op.type) {
        case TokenType::PlusEqual:  emit(OP_ADD); break;
        case TokenType::MinusEqual: emit(OP_SUB); break;
        case TokenType::MultEqual:  emit(OP_MUL); break;
        case TokenType::DivEqual:   emit(OP_DIV); break;
        case TokenType::ModEqual:   emit(OP_MOD); break;
        default:
            throw std::runtime_error("Unsupported compound assignment in codegen: " + op.value);
    }
}

int Codegen::emit_jump(OpCode op){
    emit(op);
    emit(static_cast<uint16_t>(0xFFFF)); // placeholder
    return static_cast<int>(curr->chunk.code.size()) - 2;
}

void Codegen::patch_jump(int pos){
    int off = static_cast<int>(curr->chunk.code.size()) - (pos + 2);
    curr->chunk.code[pos]     = static_cast<uint8_t>((off >> 8) & 0xFF);
    curr->chunk.code[pos + 1] = static_cast<uint8_t>(off & 0xFF);
}

void Codegen::emit_loop(int loop_start) {
    emit(OP_JUMP);
    emit(static_cast<uint16_t>(loop_start - (static_cast<int>(curr->chunk.code.size()) + 2)));
}

void Codegen::declare_variable(const Token &name) {
    scopes->declare_variable(name);
}

void Codegen::mark_initialized() {
    scopes->mark_initialized();
}

void Codegen::define_variable(const Token &name) {
    if (scopes->depth() > 0) {
        mark_initialized();
        return;
    }

    uint16_t name_idx = make_constant(name.value);
    emit(OP_DEFINE_GLOBAL);
    emit(name_idx);
}

void Codegen::define_map_method() {
    // arguments are on the stack:
    // - this function (local 0)
    // - array         (local 1)
    // - predicate     (local 2)

    // make a new array (local 3)
    emit(OP_MAKE_ARRAY);
    emit_iconst8(0); // zero elements

    // make a counter (local 4)
    emit_iconst8(0); // counter = 0

    // while counter < length of array
    int loop_start = static_cast<int>(curr->chunk.code.size());
    emit(OP_LOAD_LOCAL); emit_iconst8(4); // load counter
    emit(OP_LOAD_GLOBAL); emit(make_constant("len")); // load len function
    emit(OP_LOAD_LOCAL); emit_iconst8(1); // load array
    emit(OP_CALL); emit_iconst8(1); // call len with 1 argument
    // compare
    emit(OP_LT);
    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit(OP_POP); // pop comparison result

    emit(OP_LOAD_LOCAL); emit_iconst8(3); // load new array
    emit(OP_LOAD_FIELD); emit(make_constant("push")); // load push method
    
    emit(OP_LOAD_LOCAL); emit_iconst8(2); // load predicate
    emit(OP_LOAD_LOCAL); emit_iconst8(1); // load array
    emit(OP_LOAD_LOCAL); emit_iconst8(4); // load counter
    emit(OP_LOAD_INDEX); // index into array
    emit(OP_CALL); emit_iconst8(1); // call predicate with 1 argument

    emit(OP_CALL); emit_iconst8(1); // call push with 1 argument
    emit(OP_POP); // pop result of push

    // increment counter
    emit(OP_LOAD_LOCAL); emit_iconst8(4); // load counter
    emit_iconst8(1);
    emit(OP_ADD);
    emit(OP_STORE_LOCAL); emit_iconst8(4); // store counter
    emit(OP_POP); // pop old counter value

    emit_loop(loop_start);
    patch_jump(exit_jump);
    emit(OP_POP); // pop comparison result

    // load new array as result
    emit(OP_LOAD_LOCAL); emit_iconst8(3);
    emit(OP_RETURN);
}

void Codegen::emit_closure(const Function::Ptr &func, const std::vector<ScopeManager::Upvalue> &upvalues) {
    uint16_t func_idx = make_constant(func);
    emit(OP_CLOSURE);
    emit(func_idx);

    for (const auto &upvalue : upvalues) {
        emit(static_cast<uint8_t>(upvalue.is_local ? 1 : 0));
        emit(static_cast<uint8_t>(upvalue.index));
    }
}

void Codegen::generate_expr(const ExprStmt &stmt) {
    generate(*stmt.expr); // evaluate expression
    emit(OP_POP);         // discard result
}

void Codegen::generate_disp(const DispStmt &stmt) {
    generate(*stmt.expr); // push value to display
    emit(OP_PRINT);       // VM prints top of stack
}

void Codegen::generate_let(const LetStmt &stmt) {
    declare_variable(stmt.name);
    if (stmt.initializer) {
        generate(*stmt.initializer);   // push initializer value
    } else {
        emit(OP_NULL);    // push default null value
    }

    define_variable(stmt.name);
}

void Codegen::generate_block(const BlockStmt &stmt) {
    begin_scope();

    for (const auto &s : *stmt.statements) {
        generate(*s);
    }

    end_scope();
}

void Codegen::generate_if(const IfStmt &stmt) {
    generate(*stmt.condition); // evaluate condition
    int jump_pos = emit_jump(OP_JUMP_IF_FALSE);
    emit(OP_POP); // pop condition value

    generate(*stmt.then_branch);

    int else_jump = emit_jump(OP_JUMP);
    patch_jump(jump_pos);
    emit(OP_POP); // pop condition value

    if (stmt.else_branch) {
        generate(*stmt.else_branch);
    }

    patch_jump(else_jump);
}

void Codegen::generate_while(const WhileStmt &stmt) {
    int loop_start = static_cast<int>(curr->chunk.code.size());
    generate(*stmt.condition); // evaluate condition
    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit(OP_POP); // pop condition value

    generate(*stmt.body);
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit(OP_POP); // pop condition value
}

void Codegen::generate_function(const FunctionStmt &stmt) {
    declare_variable(stmt.name);
    mark_initialized();

    // Start compiling nested function into a new State
    begin_function(stmt.name.value, static_cast<int>(stmt.params.size()));
    begin_scope();

    // Declare and initialize parameters as locals in the nested state
    for (const auto &param : stmt.params) {
        declare_variable(param); // adds to state->Locals
        mark_initialized();
    }

    // Compile the body of the nested function
    for (const auto &s : *stmt.body) {
        generate(*s);
    }

    end_scope();

    // Capture the upvalues from the nested function's scope manager
    auto nested_upvalues = scopes->upvalues;

    // Finish the nested function and retrieve the completed FunctionPtr.
    auto finished_func = end_function();

    // Emit the closure creation in the enclosing function
    emit_closure(finished_func, nested_upvalues);

    // Finally, define the function name in the parent scope (local or global)
    define_variable(stmt.name);
}

void Codegen::generate_return(const ReturnStmt &stmt) {
    if (stmt.value) {
        generate(*stmt.value); // push return value
    } else {
        emit(OP_NULL); // default return value
    }

    emit(OP_RETURN);
}

void Codegen::generate_struct(const StructStmt &stmt) {
    declare_variable(stmt.name);

    // Emit struct creation opcode
    uint16_t name_idx = make_constant(stmt.name.value);
    emit(OP_STRUCT);
    emit(name_idx);

    // Define the struct name in the current scope
    define_variable(stmt.name);

    // Load the struct onto the stack to add methods
    emit(OP_LOAD_GLOBAL);
    emit(name_idx);

    for (const auto &m : stmt.methods) {
        auto method = std::get<FunctionStmt>(*m);

        declare_variable(method.name);

        // Compile method function
        begin_function(method.name.value, static_cast<int>(method.params.size()), true);
        begin_scope();

        // Declare and initialize parameters as locals in the method
        for (const auto &param : method.params) {
            declare_variable(param); // adds to state->Locals
            mark_initialized();
        }

        // Compile the body of the method
        for (const auto &s : *method.body) {
            generate(*s);
        }

        end_scope();

        // Capture the upvalues from the method's scope manager
        auto method_upvalues = scopes->upvalues;

        // Finish the method function and retrieve the completed FunctionPtr.
        auto finished_method = end_function(method.name.value == "init");

        // Emit the closure creation for the method
        emit_closure(finished_method, method_upvalues);

        uint16_t method_name_idx = make_constant(method.name.value);
        emit(OP_METHOD);
        emit(method_name_idx);
    }

    // Pop the struct from the stack after adding methods
    emit(OP_POP);
}

void Codegen::generate_binary(const BinaryExpr &expr) {
    generate(*expr.left);
    generate(*expr.right);

    switch (expr.op.type) {
        case TokenType::Plus:         emit(OP_ADD); break;
        case TokenType::Minus:        emit(OP_SUB); break;
        case TokenType::Mult:         emit(OP_MUL); break;
        case TokenType::Div:          emit(OP_DIV); break;
        case TokenType::Mod:          emit(OP_MOD); break;
        case TokenType::Greater:      emit(OP_GT);  break;
        case TokenType::Less:         emit(OP_LT);  break;
        case TokenType::GreaterEqual: emit(OP_GE);  break;
        case TokenType::LessEqual:    emit(OP_LE);  break;
        case TokenType::Equal:        emit(OP_EQ);  break;
        case TokenType::NotEqual:     emit(OP_NEQ); break;
        default:
            throw std::runtime_error("Unknown binary operator in codegen: " + expr.op.value);
    }
}

void Codegen::generate_logical(const LogicalExpr &expr) {
    generate(*expr.left);

    if (expr.op.type == TokenType::Or) {
        int jump_pos = emit_jump(OP_JUMP_IF_TRUE);
        emit(OP_POP); // pop left value
        generate(*expr.right);
        patch_jump(jump_pos);
    } else if (expr.op.type == TokenType::And) {
        int jump_pos = emit_jump(OP_JUMP_IF_FALSE);
        emit(OP_POP); // pop left value
        generate(*expr.right);
        patch_jump(jump_pos);
    } else {
        throw std::runtime_error("Unknown logical operator in codegen: " + expr.op.value);
    }
}

void Codegen::generate_unary(const UnaryExpr &expr) {
    switch (expr.op.type) {
        case TokenType::Not: {
            generate(*expr.right);
            emit(OP_NOT);
            break;
        }
        case TokenType::Minus: {
            generate(*expr.right);
            emit(OP_NEG);
            break;
        }
        case TokenType::BitNot: {
            generate(*expr.right);
            emit(OP_BIT_NOT);
            break;
        }
        case TokenType::Increment:
        case TokenType::Decrement: {
            // pre-increment/decrement
            auto op_type = expr.op.type == TokenType::Increment ? OP_ADD : OP_SUB;
            if (auto var = std::get_if<VariableExpr>(&*expr.right)) {
                emit_load_var(var->name);
                emit_iconst8(1);
                emit(op_type);
                emit_store_var(var->name);
            }
            else if (auto idx = std::get_if<IndexExpr>(&*expr.right)) {
                generate(*idx->target);
                generate(*idx->index);
                emit(OP_DUP2);
                emit(OP_LOAD_INDEX);
                emit_iconst8(1);
                emit(op_type);
                emit(OP_STORE_INDEX);
            }
            else if (auto dot = std::get_if<DotExpr>(&*expr.right)) {
                generate(*dot->target);
                emit(OP_DUP);
                uint16_t field_idx = make_constant(dot->key.value);
                emit(OP_LOAD_FIELD);
                emit(field_idx);
                emit_iconst8(1);
                emit(op_type);
                emit(OP_STORE_FIELD);
                emit(field_idx);
            }
            else {
                throw std::runtime_error("Invalid target for unary operator");
            }
            break;
        }
        default:
            throw std::runtime_error("Unknown unary operator in codegen");
    }
}

void Codegen::generate_postfix(const PostfixExpr &expr) {
    // post-increment/decrement
    auto op_type = expr.op.type == TokenType::Increment ? OP_ADD : OP_SUB;
    if (auto var = std::get_if<VariableExpr>(&*expr.left)) {
        emit_load_var(var->name);
        emit(OP_DUP);
        emit_iconst8(1);
        emit(op_type);
        emit_store_var(var->name);
        emit(OP_POP);
    }
    else if (auto idx = std::get_if<IndexExpr>(&*expr.left)) {
        generate(*idx->target);
        generate(*idx->index);
        emit(OP_DUP2);
        emit(OP_LOAD_INDEX);
        emit_iconst8(1);
        emit(op_type);
        emit(OP_STORE_INDEX);
        emit_iconst8(1);
        emit(op_type == OP_ADD ? OP_SUB : OP_ADD);
    }
    else if (auto dot = std::get_if<DotExpr>(&*expr.left)) {
        generate(*dot->target);
        emit(OP_DUP);
        uint16_t field_idx = make_constant(dot->key.value);
        std::cout << "Generating dot access for field: " << dot->key.value << " (const idx " << field_idx << ")\n";
        emit(OP_LOAD_FIELD);
        emit(field_idx);
        emit_iconst8(1);
        emit(op_type);
        emit(OP_STORE_FIELD);
        emit(field_idx);
        emit_iconst8(1);
        emit(op_type == OP_ADD ? OP_SUB : OP_ADD); // reverse the operation to get original value
    }
    else {
        throw std::runtime_error("Invalid target for postfix operator");
    }
}

void Codegen::generate_literal(const LiteralExpr &expr) {
    if (expr.literal.is_int()) {
        int num = expr.literal.as_int();
        if (num >= INT8_MIN && num <= INT8_MAX) {
            emit_iconst8(static_cast<int8_t>(num));
            return;
        } else if (num >= INT16_MIN && num <= INT16_MAX) {
            emit_iconst16(static_cast<int16_t>(num));
            return;
        }
    }

    emit_constant(expr.literal);
}

void Codegen::generate_variable(const VariableExpr &expr) {
    emit_load_var(expr.name);
}

void Codegen::generate_grouping(const GroupingExpr &expr) {
    generate(*expr.grouped);
}

void Codegen::generate_assign(const AssignExpr &expr) {
    if (expr.op.type == TokenType::Assign) {
        generate(*expr.value);             // push RHS
    } else {
        emit_load_var(expr.name);          // push LHS value
        generate(*expr.value);             // push RHS
        emit_compound_op(expr.op);         // apply +, -, *, / etc.
    }

    emit_store_var(expr.name);
}

void Codegen::generate_set_dot(const SetDotExpr &expr) {
    generate(*expr.target);                // push container

    uint16_t field_idx = make_constant(expr.key.value);

    if (expr.op.type == TokenType::Assign) {
        generate(*expr.value);             // push RHS
    } else {
        emit(OP_DUP);              // duplicate container for reload
        emit(OP_LOAD_FIELD);       // load container.key
        emit(field_idx);
        generate(*expr.value);             // push RHS
        emit_compound_op(expr.op);         // apply op
    }

    emit(OP_STORE_FIELD);          // container.key = value
    emit(field_idx);
}

void Codegen::generate_set_index(const SetIndexExpr &expr) {
    generate(*expr.target);                // push container
    generate(*expr.index);                 // push index

    if (expr.op.type == TokenType::Assign) {
        generate(*expr.value);             // push RHS
    } else {
        emit(OP_DUP2);             // duplicate target + index (for reload)
        emit(OP_LOAD_INDEX);       // load container[index] → push value
        generate(*expr.value);             // push RHS
        emit_compound_op(expr.op);         // apply op
    }

    emit(OP_STORE_INDEX);          // container[index] = top of stack
}

void Codegen::generate_call(const CallExpr &expr) {
    generate(*expr.callee);          // push function

    for (const auto &arg : expr.args) {
        generate(*arg);              // push arguments in order
    }

    emit(OP_CALL);
    emit(static_cast<uint8_t>(expr.args.size()));
}

void Codegen::generate_array(const ArrayExpr &expr) {
    for (const auto &el : expr.elements) {
        generate(*el);               // push each element
    }

    emit(OP_MAKE_ARRAY);
    emit(static_cast<uint16_t>(expr.elements.size()));
}

void Codegen::generate_object(const ObjectExpr &expr) {
    for (auto &[key, val] : expr.items) {
        generate(*val);              // push value
        uint16_t key_idx = make_constant(key);
        emit(OP_CONST); // push key
        emit(key_idx);
    }

    emit(OP_MAKE_OBJECT);
    emit(static_cast<uint16_t>(expr.items.size()));
}

void Codegen::generate_index(const IndexExpr &expr) {
    generate(*expr.target);        // push container
    generate(*expr.index);         // push index
    emit(OP_LOAD_INDEX);   // push container[index]
}

void Codegen::generate_dot(const DotExpr &expr) {
    generate(*expr.target);         // push container
    uint16_t field_idx = make_constant(expr.key.value);
    emit(OP_LOAD_FIELD);    // push container.key
    emit(field_idx);
}

void Codegen::generate_ternary(const TernaryExpr &expr) {
    generate(*expr.condition);      // push condition
    int jump_else = emit_jump(OP_JUMP_IF_FALSE);

    emit(OP_POP);           // pop condition
    generate(*expr.left);           // push true branch
    int jump_end = emit_jump(OP_JUMP);
    patch_jump(jump_else);
    
    emit(OP_POP);           // pop condition
    generate(*expr.right);          // push false branch
    patch_jump(jump_end);
}

void Codegen::generate_lambda(const LambdaExpr &expr) {
    // Start compiling nested function into a new State
    begin_function("_", static_cast<int>(expr.params.size()));
    begin_scope();

    // Declare and initialize parameters as locals in the nested state
    for (const auto &param : expr.params) {
        declare_variable(param); // adds to state->Locals
        mark_initialized();
    }

    // Compile the body of the nested function
    for (const auto &s : *expr.body) {
        generate(*s);
    }

    end_scope();

    // Capture the upvalues from the nested function's scope manager
    auto nested_upvalues = scopes->upvalues;

    // Finish the nested function and retrieve the completed FunctionPtr.
    auto finished_func = end_function();

    // Emit the closure creation in the enclosing function
    emit_closure(finished_func, nested_upvalues);
}

void Codegen::generate_self(const SelfExpr &expr) {
    emit_load_var(expr.keyword);
}

void Codegen::generate_spawn(const SpawnExpr &expr) {
    begin_function("lambda_spawn", 0);
    begin_scope();

    for (const auto &s : *expr.statements) {
        generate(*s);
    }

    end_scope();

    auto nested_upvalues = scopes->upvalues;
    auto finished_func = end_function();

    emit_closure(finished_func, nested_upvalues);
    
    if (expr.count) {
        generate(*expr.count);
    } else {
        emit_iconst8(1); // default to spawning 1 thread
    }

    emit(OP_SPAWN);
}

void Codegen::disassemble_function(const Function::Ptr &func) {
    const auto &code = func->chunk.code;
    const auto &consts = func->chunk.constants;
    size_t pc = 0;

    auto ensure_bytes = [&](size_t need) -> bool {
        return pc + need <= code.size();
    };

    auto read_u8 = [&](size_t offset = 1) -> uint8_t {
        return code[pc + offset];
    };
    auto read_u16 = [&](size_t offset = 1) -> uint16_t {
        return (code[pc + offset] << 8) | code[pc + offset + 1];
    };

    auto print_addr = [&](size_t addr) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << addr << " " << std::dec;
    };

    auto print_line = [&](const std::string &name, const std::string &extra = "") {
        std::cout << std::left << std::setw(15) << std::setfill(' ') << name << std::right;
        if (!extra.empty()) std::cout << " " << extra;
        std::cout << "\n";
    };

    auto truncated = [&](const std::string &name) {
        print_line(name, "<truncated>");
        pc = code.size();
    };

    while (pc < code.size()) {
        OpCode op = static_cast<OpCode>(code[pc]);
        print_addr(pc);

        switch (op) {
            // --- 1-byte operand ---
            case OP_ICONST8: {
                if (!ensure_bytes(2)) return truncated("ICONST8");
                int8_t val = static_cast<int8_t>(read_u8());
                print_line("ICONST8", std::to_string(val));
                pc += 2;
                break;
            }

            case OP_LOAD_LOCAL: case OP_STORE_LOCAL:
            case OP_LOAD_UPVALUE: case OP_STORE_UPVALUE:
            case OP_CALL: {
                if (!ensure_bytes(2)) {
                    const char* opname = (op == OP_LOAD_LOCAL) ? "LOAD_LOCAL" :
                                        (op == OP_STORE_LOCAL) ? "STORE_LOCAL" :
                                        (op == OP_LOAD_UPVALUE) ? "LOAD_UPVALUE" :
                                        (op == OP_STORE_UPVALUE) ? "STORE_UPVALUE" : "CALL";
                    return truncated(opname);
                }
                uint8_t operand = read_u8();
                const char* opname = (op == OP_LOAD_LOCAL) ? "LOAD_LOCAL" :
                                    (op == OP_STORE_LOCAL) ? "STORE_LOCAL" :
                                    (op == OP_LOAD_UPVALUE) ? "LOAD_UPVALUE" :
                                    (op == OP_STORE_UPVALUE) ? "STORE_UPVALUE" : "CALL";
                print_line(opname, std::to_string(operand));
                pc += 2;
                break;
            }

            // --- 2-byte operand ---
            case OP_DEFINE_GLOBAL: case OP_CONST:
            case OP_ICONST16: case OP_LOAD_GLOBAL:
            case OP_STORE_GLOBAL: case OP_LOAD_FIELD:
            case OP_STORE_FIELD: case OP_MAKE_ARRAY:
            case OP_MAKE_OBJECT: case OP_JUMP:
            case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE:
            case OP_CLOSURE: case OP_STRUCT: case OP_METHOD: {
                if (!ensure_bytes(3)) return truncated("TRUNCATED_OPERAND");

                int operand = read_u16();
                const char *opname = nullptr;
                switch (op) {
                    case OP_DEFINE_GLOBAL: opname = "DEFINE_GLOBAL"; break;
                    case OP_CONST:         opname = "CONST"; break;
                    case OP_ICONST16:      opname = "ICONST16"; break;
                    case OP_LOAD_GLOBAL:   opname = "LOAD_GLOBAL"; break;
                    case OP_STORE_GLOBAL:  opname = "STORE_GLOBAL"; break;
                    case OP_LOAD_FIELD:    opname = "LOAD_FIELD"; break;
                    case OP_STORE_FIELD:   opname = "STORE_FIELD"; break;
                    case OP_MAKE_ARRAY:    opname = "MAKE_ARRAY"; break;
                    case OP_MAKE_OBJECT:   opname = "MAKE_OBJECT"; break;
                    case OP_JUMP:          opname = "JUMP"; break;
                    case OP_JUMP_IF_FALSE: opname = "JUMP_IF_FALSE"; break;
                    case OP_JUMP_IF_TRUE:  opname = "JUMP_IF_TRUE"; break;
                    case OP_CLOSURE:       opname = "CLOSURE"; break;
                    case OP_STRUCT:        opname = "STRUCT"; break;
                    case OP_METHOD:        opname = "METHOD"; break;
                    default: opname = "UNKNOWN_U16"; break;
                }

                std::ostringstream extra;
                extra << operand;

                // context-specific additions
                auto append_const = [&](int idx) {
                    if (idx < static_cast<int>(consts.size()))
                        extra << " (" << consts[idx].to_string() << ")";
                };

                if (op == OP_CONST || op == OP_LOAD_GLOBAL || op == OP_STORE_GLOBAL ||
                    op == OP_DEFINE_GLOBAL || op == OP_STRUCT || op == OP_METHOD) {
                    append_const(operand);
                } else if (op == OP_ICONST16) {
                    extra << " " << static_cast<int16_t>(operand);
                } else if (op == OP_CLOSURE) {
                    append_const(operand);
                    auto closed_func = consts[operand].as_function();
                    for (size_t i = 0; i < closed_func->upvalue_count; ++i) {
                        // each upvalue has 2 bytes: is_local (1 byte) + index (1 byte)
                        if (!ensure_bytes(3 + i * 2 + 2)) {
                            extra << "<truncated>";
                            break;
                        }
                        uint8_t is_local = read_u8(3 + i * 2);
                        uint8_t index = read_u8(3 + i * 2 + 1);
                        extra << ", " << (is_local ? "local" : "upvalue") << " " << static_cast<int>(index);
                    }
                    pc += closed_func->upvalue_count * 2;
                } else if (op == OP_LOAD_FIELD || op == OP_STORE_FIELD) {
                    append_const(operand);
                }

                print_line(opname, extra.str());
                pc += 3;
                break;
            }

            // --- zero-operand ---
            default: {
                const char *opname = nullptr;
                switch (op) {
                    case OP_NULL: opname = "NULL"; break;
                    case OP_TRUE: opname = "TRUE"; break;
                    case OP_FALSE: opname = "FALSE"; break;
                    case OP_ADD: opname = "ADD"; break;
                    case OP_SUB: opname = "SUB"; break;
                    case OP_MUL: opname = "MUL"; break;
                    case OP_DIV: opname = "DIV"; break;
                    case OP_MOD: opname = "MOD"; break;
                    case OP_NOT: opname = "NOT"; break;
                    case OP_NEG: opname = "NEG"; break;
                    case OP_EQ: opname = "EQ"; break;
                    case OP_NEQ: opname = "NEQ"; break;
                    case OP_LT: opname = "LT"; break;
                    case OP_LE: opname = "LE"; break;
                    case OP_GT: opname = "GT"; break;
                    case OP_GE: opname = "GE"; break;
                    case OP_BIT_OR: opname = "BIT_OR"; break;
                    case OP_BIT_AND: opname = "BIT_AND"; break;
                    case OP_BIT_NOT: opname = "BIT_NOT"; break;
                    case OP_BIT_XOR: opname = "BIT_XOR"; break;
                    case OP_SHIFT_LEFT: opname = "SHIFT_LEFT"; break;
                    case OP_SHIFT_RIGHT: opname = "SHIFT_RIGHT"; break;
                    case OP_DUP: opname = "DUP"; break;
                    case OP_DUP2: opname = "DUP2"; break;
                    case OP_LOAD_INDEX: opname = "LOAD_INDEX"; break;
                    case OP_STORE_INDEX: opname = "STORE_INDEX"; break;
                    case OP_POP: opname = "POP"; break;
                    case OP_PRINT: opname = "PRINT"; break;
                    case OP_RETURN: opname = "RETURN"; break;
                    case OP_CLOSE_UPVALUE: opname = "CLOSE_UPVALUE"; break;
                    default: break;
                }

                if (opname) print_line(opname);
                else print_line("UNKNOWN", std::to_string(int(code[pc])));
                pc += 1;
                break;
            }
        }
    }

    // disassemble nested functions and struct methods
    for (const auto &constant : func->chunk.constants) {
        if (constant.is_function()) {
            auto fn = constant.as_function();
            std::cout << "\n== Disassembly of function: " << fn->name << " ==\n";
            disassemble_function(fn);
        } else if (constant.is_struct()) {
            auto st = constant.as_struct();
            for (const auto &method_pair : st->methods) {
                const auto &method_name = method_pair.first;
                const auto &method_func = method_pair.second.as_function();
                std::cout << "\n== Disassembly of method: " << st->name << "." << method_name << " ==\n";
                disassemble_function(method_func);
            }
        }
    }
}

void Codegen::disassemble() {
    std::cout << "== Disassembly of function: " << curr->name << " ==\n";
    disassemble_function(curr);
}