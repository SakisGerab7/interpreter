#pragma once

#include "value.hpp"
#include "runtime.hpp"

// Forward-declare dependent types
struct SelectFrame;

struct CallFrame {
    Closure* closure;
    int ip = 0; // instruction pointer
    int base = 0; // base index in the VM stack

    CallFrame() = default;
    CallFrame(Closure* c, int ip = 0, int base = 0) : closure(std::move(c)), ip(ip), base(base) {}
};

struct ExecutionContext {
    // Fixed-size stack for simplicity; can be made dynamic if needed
    std::array<Value, 512> stack;
    size_t stack_size = 0;

    // Call frames for function calls
    std::vector<CallFrame> frames;

    // Open upvalues for closures
    std::vector<Upvalue*> open_upvalues;

    inline const Value& peek_stack(size_t depth = 0) const {
        if (depth >= stack_size) throw std::runtime_error("Stack underflow on peek");
        return stack[stack_size - 1 - depth];
    }

    inline void poke_stack(const Value &v, size_t depth = 0) {
        if (depth >= stack_size) throw std::runtime_error("Stack underflow on poke");
        stack[stack_size - 1 - depth] = v;
    }
};

struct GreenThread : public Object {
    size_t ID;

    enum State {
        Running,
        Ready,
        Blocked,
        Finished,
    } state = Ready;

    // For sleep operations
    std::chrono::steady_clock::time_point wake_time;

    // Each thread has its own execution context (stack, call frames, etc.)
    ExecutionContext ctx;

    // If true, the thread's resources can be automatically reclaimed when it finishes, and it cannot be joined
    bool detached = false;

    // For join operations, store the return value of the thread when it finishes
    Value return_value;

    // For join operations
    std::vector<GreenThread*> joiners;

    // For thread hierarchy (optional, can be used for cleanup)
    GreenThread* parent = nullptr;
    std::vector<GreenThread*> children;

    // For pipe operations
    Value pending_value;

    // For select operations
    std::unique_ptr<SelectFrame> active_select;

    GreenThread(size_t id = 0, Closure* closure = nullptr);
    ~GreenThread();

    std::string type_name() const override { return "Thread"; }
    std::string to_string() const override { return "<thread " + std::to_string(ID) + ">"; }
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_thread(this); }
    size_t object_size() const override;
};
