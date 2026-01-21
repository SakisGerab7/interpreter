#include "bytecode.hpp"
#include "value.hpp"

uint16_t Chunk::add_constant(const Value &v) {
    for (uint16_t i = 0; i < constants.size(); i++) {
        if (constants[i] == v) return i;
    }

    constants.push_back(v);
    return static_cast<uint16_t>(constants.size() - 1);
}

std::string opcode_to_string(OpCode op) {
    switch (op) {
        case OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
        case OP_NULL: return "NULL";
        case OP_TRUE: return "TRUE";
        case OP_FALSE: return "FALSE";
        case OP_CONST: return "CONST";
        case OP_ICONST8: return "ICONST8";
        case OP_ICONST16: return "ICONST16";
        case OP_CLOSURE: return "CLOSURE";
        case OP_LOAD_LOCAL: return "LOAD_LOCAL";
        case OP_STORE_LOCAL: return "STORE_LOCAL";
        case OP_LOAD_UPVALUE: return "LOAD_UPVALUE";
        case OP_STORE_UPVALUE: return "STORE_UPVALUE";
        case OP_CLOSE_UPVALUE: return "CLOSE_UPVALUE";
        case OP_LOAD_GLOBAL: return "LOAD_GLOBAL";
        case OP_STORE_GLOBAL: return "STORE_GLOBAL";
        case OP_LOAD_INDEX: return "LOAD_INDEX";
        case OP_STORE_INDEX: return "STORE_INDEX";
        case OP_LOAD_FIELD: return "LOAD_FIELD";
        case OP_STORE_FIELD: return "STORE_FIELD";
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_NOT: return "NOT";
        case OP_NEG: return "NEG";
        case OP_EQ: return "EQ";
        case OP_NEQ: return "NEQ";
        case OP_LT: return "LT";
        case OP_LE: return "LE";
        case OP_GT: return "GT";
        case OP_GE: return "GE";
        case OP_BIT_OR: return "BIT_OR";
        case OP_BIT_AND: return "BIT_AND";
        case OP_BIT_NOT: return "BIT_NOT";
        case OP_BIT_XOR: return "BIT_XOR";
        case OP_SHIFT_LEFT: return "SHIFT_LEFT";
        case OP_SHIFT_RIGHT: return "SHIFT_RIGHT";
        case OP_DUP: return "DUP";
        case OP_DUP2: return "DUP2";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OP_CALL: return "CALL";
        case OP_MAKE_ARRAY: return "MAKE_ARRAY";
        case OP_MAKE_OBJECT: return "MAKE_OBJECT";
        case OP_POP: return "POP";
        case OP_PRINT: return "PRINT";
        case OP_RETURN: return "RETURN";
        case OP_STRUCT: return "STRUCT";
        case OP_METHOD: return "METHOD";
        case OP_SPAWN: return "SPAWN";
        case OP_GET_ITER: return "GET_ITER";
        case OP_ITER_NEXT: return "ITER_NEXT";
        case OP_LOAD_ITER_INDEX: return "LOAD_ITER_INDEX";
        case OP_SEND_PIPE: return "SEND_PIPE";
        case OP_RECV_PIPE: return "RECV_PIPE";
        case OP_CLOSE_PIPE: return "CLOSE_PIPE";
        case OP_SELECT_BEGIN: return "SELECT_BEGIN";
        case OP_SELECT_RECV: return "SELECT_RECV";
        case OP_SELECT_SEND: return "SELECT_SEND";
        case OP_SELECT_DEFAULT: return "SELECT_DEFAULT";
        case OP_SELECT_EXEC: return "SELECT_EXEC";
        default: return "UNKNOWN";
    }
}
