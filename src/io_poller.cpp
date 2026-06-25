#include "io_poller.hpp"
#include <iostream>

EpollPoller::EpollPoller() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

EpollPoller::~EpollPoller() {
    close(epoll_fd);
}

void EpollPoller::add_handle(IOHandle* handle) {
    struct epoll_event event;
    event.data.fd = handle->fd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET; // Edge-triggered for better performance

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handle->fd, &event) == -1) {
        throw std::runtime_error("Failed to add io_handler to epoll");
    }

    handle_map[handle->fd] = handle;
}

void EpollPoller::remove_handle(IOHandle* handle) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle->fd, nullptr) == -1) {
        throw std::runtime_error("Failed to remove io_handler from epoll");
    }

    handle_map.erase(handle->fd);
}

void EpollPoller::poll(int timeout_ms, std::function<void(const IOEvent& event)> event_callback) {
    std::vector<IOEvent> events;
    static constexpr int MAX_EVENTS = 64;
    struct epoll_event epoll_events[MAX_EVENTS];

    int num_events = epoll_wait(epoll_fd, epoll_events, MAX_EVENTS, timeout_ms);
    if (num_events == -1) {
        throw std::runtime_error("Failed to wait for epoll events");
    }

    for (int i = 0; i < num_events; ++i) {
        int fd = epoll_events[i].data.fd;
        auto it = handle_map.find(fd);
        if (it != handle_map.end()) {
            IOHandle* handle = it->second;
            if (epoll_events[i].events & EPOLLIN) {
                event_callback(IOEvent{IOEvent::Type::Readable, handle});
            }
            if (epoll_events[i].events & EPOLLOUT) {
                event_callback(IOEvent{IOEvent::Type::Writable, handle});
            }
            if (epoll_events[i].events & (EPOLLERR | EPOLLHUP)) {
                event_callback(IOEvent{IOEvent::Type::Error, handle});
            }
        }
    }
}

SelectPoller::SelectPoller() {
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);
}

SelectPoller::~SelectPoller() {
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);
}

void SelectPoller::add_handle(IOHandle* handle) {
    handle_map[handle->fd] = handle;

    if (handle->fd > max_fd) {
        max_fd = handle->fd;
    }

    FD_SET(handle->fd, &read_fds);
    FD_SET(handle->fd, &write_fds);
    FD_SET(handle->fd, &error_fds);
}

void SelectPoller::remove_handle(IOHandle* handle) {
    handle_map.erase(handle->fd);

    if (handle->fd == max_fd) {
        max_fd = -1;
        for (const auto& [fd, _] : handle_map) {
            if (fd > max_fd) {
                max_fd = fd;
            }
        }
    }

    FD_CLR(handle->fd, &read_fds);
    FD_CLR(handle->fd, &write_fds);
    FD_CLR(handle->fd, &error_fds);
}

void SelectPoller::poll(int timeout_ms, std::function<void(const IOEvent& event)> event_callback) {
    fd_set read_fds_copy = read_fds;
    fd_set write_fds_copy = write_fds;
    fd_set error_fds_copy = error_fds;

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int num_events = select(max_fd + 1, &read_fds_copy, &write_fds_copy, &error_fds_copy, &timeout);
    if (num_events == -1) {
        throw std::runtime_error("Failed to wait for select events: " + std::string(strerror(errno)));
    }

    for (const auto& [fd, handle] : handle_map) {
        if (FD_ISSET(fd, &read_fds_copy)) {
            event_callback(IOEvent{IOEvent::Type::Readable, handle});
        }
        if (FD_ISSET(fd, &write_fds_copy)) {
            event_callback(IOEvent{IOEvent::Type::Writable, handle});
        }
        if (FD_ISSET(fd, &error_fds_copy)) {
            event_callback(IOEvent{IOEvent::Type::Error, handle});
        }
    }
}
