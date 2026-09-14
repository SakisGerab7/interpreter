#pragma once

#include "scheduler.hpp"
#include "green_thread.hpp"
#include "memory.hpp"

struct VM {
    struct NativeSignature {
        std::string name;
        int arity;

        bool operator==(const NativeSignature &other) const {
            return name == other.name && arity == other.arity;
        }
    };

    struct NativeSignatureHash {
        size_t operator()(const NativeSignature &sig) const {
            size_t h1 = std::hash<std::string>{}(sig.name);
            size_t h2 = std::hash<int>{}(sig.arity);
            return h1 ^ (h2 << 1);
        }
    };

    std::unordered_map<std::string, Value> globals;
    std::unordered_map<NativeSignature, NativeFn, NativeSignatureHash> native_registry;

    Heap* heap = nullptr;

    Scheduler scheduler;
    GreenThread* current_thread = nullptr;
    GreenThread* main_thread = nullptr;

    Value cli_arguments;

    // Profiling state
    bool profile_enabled = false;
    bool profile_dumped = false;
    bool profile_time_enabled = false;
    std::array<uint64_t, 256> opcode_counts{};
    std::array<uint64_t, 256> opcode_time_ns{};
    uint64_t total_instructions = 0;
    uint64_t total_time_ns = 0;

    VM(const std::vector<std::string> &args = {}, Heap* heap_ptr = nullptr, bool initialize_runtime = true);
    static std::unique_ptr<VM> create(const std::vector<std::string> &args = {}, Heap* heap_ptr = nullptr);
    static std::unique_ptr<VM> load(const std::string &state_file, Heap* heap_ptr, const std::vector<std::string> &args = {});

    void spawn_thread(Closure* closure, size_t thread_count);

    Value interpret(Function* func);
    Value resume();

    NativeFn resolve_native(const std::string &name, int arity) const;
    void define_native(const std::string &name, int arity, NativeFn func);

    void bind_native_method(const Value &obj, const std::string &method_name);

    void call_value(const Value &callee, int arg_count);
    void call_native(Native* native, int arg_count);
    void call(Closure* closure, int arg_count);

    Upvalue* capture_upvalue(int slot_index);
    void close_upvalues(int last);

    uint8_t read_byte(CallFrame &frame);
    uint16_t read_short(CallFrame &frame);

    void push(const Value& v);
    const Value& pop() const;
    const Value& peek(size_t depth = 0) const;
    void poke(const Value &v, size_t depth = 0);

    void unary_op(OpCode op);
    void binary_op(OpCode op);

    Array* concat_array(Array* a, Array* b);
    ByteArray* concat_bytearray(ByteArray* a, ByteArray* b);

    void debug_instruction(CallFrame &frame, OpCode op);
    void dump_profile();
    void run();

    std::string stack_trace() const;
};

struct RuntimeError : public std::runtime_error {
    RuntimeError(const VM& vm, const std::string& message)
        : std::runtime_error("[Runtime Error]: " + message + "\nRunning Thread: " + vm.current_thread->to_string() + "\nStack Trace:\n" + vm.stack_trace()) {}
};
