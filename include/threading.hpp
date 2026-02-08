#pragma once

#include "common.hpp"
#include "runtime.hpp"
#include "value.hpp"

// Forward declarations
struct VM;
struct GreenThread;
struct SelectFrame;

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
    } state = Ready;
    
    // For sleep operations
    std::chrono::steady_clock::time_point wake_time;
    
    // Thread-local stack and call frames
    std::array<Value, 512> stack;
    size_t stack_size = 0;
    std::vector<CallFrame> frames;
    std::vector<Upvalue::Ptr> open_upvalues;
    
    // For join operations
    std::vector<GreenThread::Ptr> joiners;

    // For thread hierarchy (optional, can be used for cleanup)
    std::vector<GreenThread::Ptr> children;

    // For pipe operations
    Value pending_value;

    // For select operations
    std::unique_ptr<SelectFrame> active_select = nullptr;

    GreenThread(size_t id = 0) : ID(id) {}
};

struct Pipe {
    using Ptr = std::shared_ptr<Pipe>;

    size_t ID;

    size_t capacity;
    std::deque<Value> buffer;

    std::deque<GreenThread::Ptr> readers;
    std::deque<GreenThread::Ptr> writers;

    bool closed = false;

    std::vector<GreenThread::Ptr> select_waiters;

    Pipe(size_t id, size_t cap) : ID(id), capacity(cap) {}

    bool can_receive();
    bool can_send();
};

struct SelectCase {
    enum Type { Recv, Send } type;

    Pipe::Ptr pipe;
    uint8_t slot;
    Value value;
    int target_ip;
};

struct SelectFrame {
    std::vector<SelectCase> cases;
    bool has_default = false;
    int default_target_ip;
};

struct Scheduler {
    VM &vm;

    // Thread state
    size_t next_thread_id = 0;
    std::unordered_map<size_t, GreenThread::Ptr> threads;
    std::deque<GreenThread::Ptr> ready_queue;
    std::priority_queue<
        std::pair<std::chrono::steady_clock::time_point, GreenThread::Ptr>,
        std::vector<std::pair<std::chrono::steady_clock::time_point, GreenThread::Ptr>>,
        std::greater<std::pair<std::chrono::steady_clock::time_point, GreenThread::Ptr>>
    > blocked_queue;

    std::unordered_map<size_t, Value> return_values; // thread ID -> return value

    // Pipe state
    size_t next_pipe_id = 0;

    Scheduler(VM &vm_ref) : vm(vm_ref) {}

    // Thread management
    GreenThread::Ptr get_thread_by_id(size_t id);
    void add_thread(GreenThread::Ptr thread);    
    void enqueue(GreenThread::Ptr thread);
    inline GreenThread::Ptr dequeue();
    inline void block_thread(GreenThread::Ptr &thread);
    void notify_waiters(GreenThread::Ptr &thread);
    void kill_thread_and_children(GreenThread::Ptr &thread);
    Value get_return_value(size_t thread_id);
    void set_return_value(GreenThread::Ptr &thread, const Value &return_value);

    // Sleep/wake operations
    void send_to_sleep(GreenThread::Ptr thread, int ms);
    void wake_threads(const std::chrono::steady_clock::time_point &now);
    void sleep_until_ready(const std::chrono::steady_clock::time_point &now);

    // Pipe management
    void notify_pipe_select_waiters(Pipe::Ptr &pipe);

    // Pipe operations
    void send_to_pipe(Pipe::Ptr pipe, const Value &val);
    Value receive_from_pipe(Pipe::Ptr pipe);
    void close_pipe(Pipe::Ptr pipe);

    // Select operations
    void select_begin(uint8_t case_count);
    void select_add_recv_case(Pipe::Ptr pipe, uint16_t target_ip, uint8_t slot);
    void select_add_send_case(Pipe::Ptr pipe, uint16_t target_ip, Value val);
    void select_add_default_case(uint16_t target_ip);
    void select_execute(int &curr_ip);

    // Main scheduler
    void handle_thread_state(GreenThread::Ptr &next_thread);
    Value schedule();
};
