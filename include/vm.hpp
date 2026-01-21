#pragma once

#include "common.hpp"
#include "threading.hpp"

struct VM {
    std::unordered_map<std::string, Value> globals;

    Scheduler scheduler;
    GreenThread::Ptr current_thread;

    VM();

    void spawn_thread(Closure::Ptr closure, size_t thread_count);

    Value interpret(Function::Ptr func);

    inline void define_native(const std::string &name, int arity, NativeFn func);

    void call_value(const Value &callee, int arg_count);
    void call_native(const Native::Ptr &native, int arg_count);
    void call(const Closure::Ptr &closure, int arg_count);

    inline Upvalue::Ptr capture_upvalue(Value *local);
    inline void close_upvalues(int last);

    inline uint8_t read_byte(CallFrame &frame);
    inline uint16_t read_short(CallFrame &frame);

    inline void push(const Value& v);
    inline Value pop();
    inline Value& peek(size_t depth);

    inline void unary_op(OpCode op);
    inline void binary_op(OpCode op);

    void debug_instruction(CallFrame &frame, OpCode op);
    void run();
};