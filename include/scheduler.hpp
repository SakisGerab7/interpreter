#pragma once

#include "green_thread.hpp"
#include "pipe.hpp"
#include "io_handler.hpp"
#include "io_poller.hpp"

struct VM; // Forward declaration

struct Scheduler {
    VM &vm;

    // Thread state
    size_t next_thread_id = 0;

    std::deque<GreenThread*> ready_queue;

    std::priority_queue<
        std::pair<std::chrono::steady_clock::time_point, GreenThread*>,
        std::vector<std::pair<std::chrono::steady_clock::time_point, GreenThread*>>,
        std::greater<std::pair<std::chrono::steady_clock::time_point, GreenThread*>>
    > blocked_queue;

    // Pipe state
    size_t next_pipe_id = 0;

    // I/O state
    std::unique_ptr<IOPoller> io_poller;

    Scheduler(VM &vm_ref) : vm(vm_ref), io_poller(std::make_unique<SelectPoller>()) {}
    // Scheduler(VM &vm_ref) : vm(vm_ref), io_poller(std::make_unique<EpollPoller>()) {}

    // Thread management
    void enqueue(GreenThread* thread);
    inline GreenThread* dequeue();
    inline void block_thread(GreenThread* thread);
    void notify_waiters(GreenThread* thread);
    void finish_children(GreenThread* thread);

    // Sleep/wake operations
    void send_to_sleep(GreenThread* thread, int ms);
    void wake_threads(const std::chrono::steady_clock::time_point &now);
    void sleep_until_ready(const std::chrono::steady_clock::time_point &now);
    int calculate_poll_timeout(const std::chrono::steady_clock::time_point &now);

    // standard I/O handlers
    IOHandle* stdin_handle();
    IOHandle* stdout_handle();

    // I/O operations
    IOHandle* file_open(const std::string &path, const std::string &mode);

    // Socket operations
    IOHandle* socket_unix_listener(const std::string &path);
    IOHandle* socket_unix_connection(const std::string &path);
    IOHandle* socket_tcp_listener(const std::string &address, uint16_t port);
    IOHandle* socket_tcp_connection(const std::string &address, uint16_t port);

    Value client_handle_from(int client_fd, IOHandle* handle, sockaddr_storage &client_addr);

    Value io_read_num_bytes(IOHandle* handle, size_t nbytes);
    Value io_read_until_delimiter(IOHandle* handle, uint8_t delimiter);
    Value io_read_all(IOHandle* handle);
    Value io_write(IOHandle* handle, const Value &val);
    Value io_accept(IOHandle* handle);
    void io_close(IOHandle* handle);

    void complete_connects(IOHandle* handle);
    void complete_reads(IOHandle* handle);
    void complete_writes(IOHandle* handle);
    void complete_accepts(IOHandle* handle);

    void complete_read_num_bytes(IOHandle* handle, const IOOperation &pending_read);
    void complete_read_until_delimiter(IOHandle* handle, const IOOperation &pending_read);
    void complete_read_all(IOHandle* handle, const IOOperation &pending_read);
    void complete_write(IOHandle* handle, IOOperation &pending_write);
    void complete_accept(IOHandle* handle, const IOOperation &pending_accept);

    // Socket event polling
    void poll_io_events(int timeout_ms);

    // Main scheduler
    void handle_thread_state(GreenThread* &next_thread);
    Value schedule();
};
