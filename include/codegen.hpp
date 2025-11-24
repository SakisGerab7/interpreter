#pragma once

#include "common.hpp"
#include "value.hpp"
#include "ast.hpp"
#include "scope_manager.hpp"

enum OpCode : uint8_t {
    OP_DEFINE_GLOBAL,
    OP_NULL, OP_TRUE, OP_FALSE,
    OP_CONST,          // operand: constant index
    OP_ICONST8,        // operand: small signed 8-bit integer
    OP_ICONST16,       // operand: small signed 16-bit integer
    OP_LOAD_LOCAL,     // operand: variable name index
    OP_STORE_LOCAL,    // operand: variable name index
    OP_LOAD_UPVALUE,   // operand: upvalue index
    OP_STORE_UPVALUE,  // operand: upvalue index
    OP_CLOSE_UPVALUE,  // no operand
    OP_LOAD_GLOBAL,    // operand: variable name index
    OP_STORE_GLOBAL,   // operand: variable name index
    OP_LOAD_INDEX,     // pops index + container, pushes value
    OP_STORE_INDEX,    // pops value + index + container
    OP_LOAD_FIELD,     // operand: field name index
    OP_STORE_FIELD,    // operand: field name index
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NOT, OP_NEG,
    OP_EQ, OP_NEQ, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_BIT_OR, OP_BIT_AND, OP_BIT_NOT, OP_BIT_XOR, OP_SHIFT_LEFT, OP_SHIFT_RIGHT,
    OP_DUP,            // duplicate top of stack
    OP_DUP2,           // duplicate top 2 values
    OP_JUMP,           // unconditional jump, operand = jump offset
    OP_JUMP_IF_FALSE,  // pop top, if false jump
    OP_JUMP_IF_TRUE,   // pop top, if true jump
    OP_CALL,           // operand = argument count
    OP_MAKE_ARRAY, OP_MAKE_OBJECT,
    OP_POP,
    OP_PRINT,
    OP_RETURN,
    OP_CLOSURE,        // operand: function index
    OP_STRUCT,         // operand: struct index
    OP_METHOD,         // operand: method name index
    OP_SPAWN
};

struct Codegen {
    std::shared_ptr<ScopeManager> scopes = nullptr;
    Function::Ptr curr;
    std::vector<Function::Ptr> function_stack;
    
    Codegen() = default;

    Function::Ptr compile(const std::shared_ptr<std::vector<StmtPtr>> &statements);

    void begin_function(const std::string &name, int arity = 0, bool is_method = false);
    Function::Ptr end_function(bool is_init = false);

    void generate(const Expr &expr);
    void generate(const Stmt &stmt);

    inline void begin_scope();
    inline void end_scope();

    inline void emit(OpCode op)  { curr->chunk.write_u8(static_cast<uint8_t>(op)); }
    inline void emit(uint8_t b)  { curr->chunk.write_u8(b); }
    inline void emit(uint16_t v) { curr->chunk.write_u16(v); }

    inline uint16_t make_constant(const Value &v);

    void declare_variable(const Token &name);
    void mark_initialized();
    ScopeManager::ResolveResult resolve_variable(const Token &name);

    void emit_constant(const Value &v);
    void emit_iconst8(int8_t v);
    void emit_iconst16(int16_t v);
    void emit_return(bool is_init = false);
    void emit_load_var(const Token &name);
    void emit_store_var(const Token &name);
    void emit_compound_op(const Token &op);
    void emit_closure(const Function::Ptr &func, const std::vector<ScopeManager::Upvalue> &upvalues);
    
    int emit_jump(OpCode op);
    void patch_jump(int pos);
    void emit_loop(int loop_start);

    void define_variable(const Token &name);

    void define_map_method();

    void generate_expr(const ExprStmt &stmt);
    void generate_disp(const DispStmt &stmt);
    void generate_let(const LetStmt &stmt);
    void generate_block(const BlockStmt &stmt);
    void generate_if(const IfStmt &stmt);
    void generate_while(const WhileStmt &stmt);
    void generate_function(const FunctionStmt &stmt);
    void generate_return(const ReturnStmt &stmt);
    void generate_struct(const StructStmt &stmt);

    void generate_binary(const BinaryExpr &expr);
    void generate_logical(const LogicalExpr &expr);
    void generate_unary(const UnaryExpr &expr);
    void generate_postfix(const PostfixExpr &expr);
    void generate_literal(const LiteralExpr &expr);
    void generate_variable(const VariableExpr &expr);
    void generate_grouping(const GroupingExpr &expr);
    void generate_assign(const AssignExpr &expr);
    void generate_set_dot(const SetDotExpr &expr);
    void generate_set_index(const SetIndexExpr &expr);
    void generate_call(const CallExpr &expr);
    void generate_array(const ArrayExpr &expr);
    void generate_object(const ObjectExpr &expr);
    void generate_index(const IndexExpr &expr);
    void generate_dot(const DotExpr &expr);
    void generate_ternary(const TernaryExpr &expr);
    void generate_lambda(const LambdaExpr &expr);
    void generate_self(const SelfExpr &expr);
    void generate_spawn(const SpawnExpr &expr);

    void disassemble_function(const Function::Ptr &func);
    void disassemble();
};