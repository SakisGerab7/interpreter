#pragma once

#include "common.hpp"
#include "codegen.hpp"

std::string opcode_to_string(OpCode op);

struct CallFrame {
    Closure::Ptr closure;
    int ip = 0; // instruction pointer
    int base = 0; // base index in the VM stack
};

struct GreenThread {
    using Ptr = std::shared_ptr<GreenThread>;

    size_t ID;
    
    enum State {
        Running,
        Ready,
        Blocked,
        Finished,
        // Joined,
    } state = Ready;

    std::chrono::steady_clock::time_point wake_time;

    std::array<Value, 512> stack;
    size_t stack_size = 0;
    std::vector<CallFrame> frames;
    std::vector<Upvalue::Ptr> open_upvalues;

    std::vector<GreenThread::Ptr> children;

    GreenThread(size_t id = 0) : ID(id) {}
};

struct VM;

struct Scheduler {
    size_t next_thread_id = 0;
    std::unordered_map<size_t, GreenThread::Ptr> threads;
    std::deque<size_t> ready_queue;
    std::priority_queue<
        std::pair<std::chrono::steady_clock::time_point, size_t>,
        std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>>,
        std::greater<>
    > blocked_queue;

    std::unordered_map<size_t, size_t> join_map; // parent thread ID -> child thread ID
    std::unordered_map<size_t, Value> return_values; // thread ID -> return value

    GreenThread::Ptr get_thread_by_id(size_t id);
    void add_thread(GreenThread::Ptr thread);
    void enqueue(GreenThread::Ptr thread);
    inline GreenThread::Ptr dequeue();
    inline void block_thread(GreenThread::Ptr &thread);

    Value get_return_value(size_t thread_id);
    void set_return_value(GreenThread::Ptr &thread, const Value &return_value);

    void notify_waiters(GreenThread::Ptr &thread);
    void kill_thread_and_children(GreenThread::Ptr &thread);

    void wake_threads(const std::chrono::steady_clock::time_point &now);
    void sleep_until_ready(const std::chrono::steady_clock::time_point &now);
    void send_to_sleep(GreenThread::Ptr thread, int ms);

    Value schedule(VM &vm);
};

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