#pragma once

#include "common.hpp"

// Forward declaration
struct Value;

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
    OP_SPAWN,
    OP_GET_ITER,
    OP_ITER_NEXT,
    OP_LOAD_ITER_INDEX,
    OP_SEND_PIPE,
    OP_RECV_PIPE,
    OP_CLOSE_PIPE,
    OP_SELECT_BEGIN,   // operand: number of cases
    OP_SELECT_RECV,    // operand: jump offset and slot index
    OP_SELECT_SEND,    // operand: jump offset
    OP_SELECT_DEFAULT, // operand: jump offset
    OP_SELECT_EXEC
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;

    inline void write_u8(uint8_t v) {
        code.push_back(v);
    }

    inline void write_u16(uint16_t v) {
        code.push_back((v >> 8) & 0xFF);
        code.push_back(v & 0xFF);
    }

    uint16_t add_constant(const Value &v);
};

std::string opcode_to_string(OpCode op);
