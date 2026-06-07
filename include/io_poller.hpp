#pragma once

#include "io_handler.hpp"
#include <functional>

struct IOEvent {
    enum class Type {
        Readable,
        Writable,
        Error,
    } type;

    IOHandle* handle;
};

struct IOPoller {
    virtual ~IOPoller() = default;
    virtual void add_handle(IOHandle* handle) = 0;
    virtual void remove_handle(IOHandle* handle) = 0;
    virtual void poll(int timeout_ms, std::function<void(const IOEvent& event)> event_callback) = 0;
};

struct SelectPoller : public IOPoller {
    int max_fd = -1;
    fd_set read_fds, write_fds, error_fds;
    std::unordered_map<int, IOHandle*> handle_map;

    SelectPoller();
    ~SelectPoller();

    void add_handle(IOHandle* handle) override;
    void remove_handle(IOHandle* handle) override;
    void poll(int timeout_ms, std::function<void(const IOEvent& event)> event_callback) override;
};

struct EpollPoller : public IOPoller {
    int epoll_fd;
    std::unordered_map<int, IOHandle*> handle_map;

    EpollPoller();
    ~EpollPoller();

    void add_handle(IOHandle* handle) override;
    void remove_handle(IOHandle* handle) override;
    void poll(int timeout_ms, std::function<void(const IOEvent& event)> event_callback) override;
};
