#include "io_poller.hpp"
#include "vm.hpp"
#include "scheduler.hpp"

static constexpr size_t CHUNK_SIZE = 5;

void Scheduler::enqueue(GreenThread* thread) {
    ready_queue.push_back(thread);
}

GreenThread* Scheduler::dequeue() {
    if (ready_queue.empty()) return nullptr;
    auto thread = ready_queue.front();
    ready_queue.pop_front();
    return thread;
}

void Scheduler::block_thread(GreenThread* thread) {
    blocked_queue.emplace(thread->wake_time, thread);
}

void Scheduler::notify_waiters(GreenThread* thread) {
    for (auto &joiner : thread->joiners) {
        if (joiner->state != GreenThread::Finished) {
            joiner->ctx.poke_stack(thread->return_value);
            joiner->state = GreenThread::Ready;
            enqueue(joiner);
        }
    }
}

void Scheduler::finish_children(GreenThread* thread) {
    for (auto &child : thread->children) {
        if (child->state != GreenThread::Finished) {
            child->state = GreenThread::Finished;
            notify_waiters(child);
        }
    }
}

void Scheduler::send_to_sleep(GreenThread* thread, int ms) {
    // std::cout << "[Thread " << thread->ID << " sleeping for " << ms << " ms]\n";
    if (ms <= 0) {
        thread->state = GreenThread::Ready;
        return;
    }

    thread->state = GreenThread::Blocked;
    thread->wake_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
}

void Scheduler::wake_threads(const std::chrono::steady_clock::time_point &now) {
    while (!blocked_queue.empty() && blocked_queue.top().first <= now) {
        auto thread = blocked_queue.top().second;
        blocked_queue.pop();

        thread->state = GreenThread::Ready;
        thread->wake_time = {};
        enqueue(thread);
    }
}

void Scheduler::sleep_until_ready(const std::chrono::steady_clock::time_point &now) {
    if (!blocked_queue.empty()) {
        auto sleep_duration = blocked_queue.top().first - now;
        if (sleep_duration.count() > 0) {
            // std::cerr << "[Scheduler sleeping for "
            //           << std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count()
            //           << " ms]\n";
            std::this_thread::sleep_for(sleep_duration);
        }
    }
}

int Scheduler::calculate_poll_timeout(const std::chrono::steady_clock::time_point &now) {
    if (blocked_queue.empty()) {
        // No sleeping threads, poll indefinitely (or with a reasonable timeout)
        return 100; // 100ms timeout to stay responsive
    }

    auto sleep_duration = blocked_queue.top().first - now;
    if (sleep_duration.count() <= 0) {
        return 0; // Wake time is now or past, don't wait
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count();
    return static_cast<int>(ms);
}

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get file descriptor flags");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set file descriptor to non-blocking mode");
    }
}

void set_tcp_listener_options(int fd) {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Failed to set SO_REUSEPORT: " + std::string(strerror(errno)));
    }
}

void set_tcp_stream_options(int fd) {
    int opt = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Failed to set TCP_NODELAY: " + std::string(strerror(errno)));
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Failed to set SO_KEEPALIVE: " + std::string(strerror(errno)));
    }
}

IOHandle* Scheduler::stdin_handle() {
    set_non_blocking(STDIN_FILENO);

    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::Stdin;
    handle->fd = STDIN_FILENO;

    io_poller->add_handle(handle);

    return handle;
}

IOHandle* Scheduler::file_open(const std::string &path, const std::string &mode) {
    int flags = 0;
    if (mode == "r") {
        flags = O_RDONLY;
    } else if (mode == "w") {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (mode == "a") {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (mode == "rw") {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (mode == "ra") {
        flags = O_RDWR | O_CREAT | O_APPEND;
    } else {
        throw std::runtime_error("Invalid file mode: " + mode);
    }

    int fd = open(path.c_str(), flags, S_IRWXU);
    if (fd == -1) {
        throw std::runtime_error("Failed to open file: " + std::string(strerror(errno)));
    }

    set_non_blocking(fd);

    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::File;
    handle->fd = fd;
    handle->metadata.path = path;

    io_poller->add_handle(handle);

    return handle;
}

IOHandle* Scheduler::socket_unix_listener(const std::string &path) {
    // Create a Unix domain socket, bind it to the specified path, and start listening
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set the socket to non-blocking mode
    set_non_blocking(fd);

    // Set up the socket address structure
    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    // Unlink the socket path if it already exists (to handle stale sockets)
    unlink(path.c_str());

    // Bind the socket to the path
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(fd);
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }

    // Start listening for incoming connections
    if (listen(fd, SOMAXCONN) == -1) {
        close(fd);
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    // Create a IOHandler object and add it to the scheduler's I/O handler map
    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::ListenerUnix;
    handle->fd = fd;
    handle->metadata.path = path;

    io_poller->add_handle(handle);

    return handle;
}

IOHandle* Scheduler::socket_unix_connection(const std::string &path) {
    // Create a Unix domain socket and connect it to the specified path
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set the socket to non-blocking mode
    set_non_blocking(fd);

    // Set up the socket address structure
    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    // Create a IOHandler object and add it to the scheduler's I/O handler map
    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::StreamUnix;
    handle->fd = fd;
    handle->metadata.path = path;

    io_poller->add_handle(handle);

    // Connect to the server socket
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == EAGAIN || errno == EINPROGRESS) {
            handle->write_interest = true;

            IOOperation pending_connect;
            pending_connect.type = IOOperation::Type::Connect;
            pending_connect.thread = vm.current_thread;
            handle->connects.push_back(pending_connect);

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
        } else {
            close(fd);
            throw std::runtime_error("Failed to connect to socket: " + std::string(strerror(errno)));
        }
    }

    return handle;
}

IOHandle* Scheduler::socket_tcp_listener(const std::string &address, uint16_t port) {
    // Create a TCP socket, bind it to the specified address and port, and start listening
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set the socket to non-blocking mode
    set_non_blocking(fd);

    // Set socket options for better performance and reliability
    set_tcp_listener_options(fd);

    // Set up the socket address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (address == "*") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0) {
            close(fd);
            throw std::runtime_error("Invalid IP address: " + std::string(strerror(errno)));
        }
    }

    // Bind the socket to the address and port
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(fd);
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }

    // Start listening for incoming connections
    if (listen(fd, SOMAXCONN) == -1) {
        close(fd);
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }

    // Create a IOHandler object and add it to the scheduler's I/O handler map
    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::ListenerTCP;
    handle->fd = fd;
    handle->metadata.local_address = address + ":" + std::to_string(port);

    io_poller->add_handle(handle);

    return handle;
}

IOHandle* Scheduler::socket_tcp_connection(const std::string &address, uint16_t port) {
    // Create a TCP socket and connect it to the specified address and port
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set the socket to non-blocking mode
    set_non_blocking(fd);

    // Set socket options for better performance and reliability
    set_tcp_stream_options(fd);

    // Set up the socket address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0) {
        close(fd);
        throw std::runtime_error("Invalid IP address: " + std::string(strerror(errno)));
    }

    // Create a IOHandle object and add it to the scheduler's I/O handler map
    auto handle = vm.heap->allocate<IOHandle>();
    handle->kind = IOHandle::Kind::StreamTCP;
    handle->fd = fd;
    handle->metadata.remote_address = address + ":" + std::to_string(port);

    io_poller->add_handle(handle);

    // Connect to the server socket
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == EAGAIN || errno == EINPROGRESS) {
            handle->write_interest = true;

            IOOperation pending_connect;
            pending_connect.type = IOOperation::Type::Connect;
            pending_connect.thread = vm.current_thread;
            handle->connects.push_back(pending_connect);

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
        } else {
            close(fd);
            throw std::runtime_error("Failed to connect to socket: " + std::string(strerror(errno)));
        }
    }

    // Get the local address and port assigned to the socket (useful for clients that bind to an ephemeral port)
    struct sockaddr_in local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    if (getsockname(fd, (struct sockaddr*)&local_addr, &local_addr_len) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get local socket address: " + std::string(strerror(errno)));
    }

    char local_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip)) == nullptr) {
        close(fd);
        throw std::runtime_error("Failed to convert local IP address: " + std::string(strerror(errno)));
    }

    uint16_t local_port = ntohs(local_addr.sin_port);
    handle->metadata.local_address = std::string(local_ip) + ":" + std::to_string(local_port);

    return handle;
}

Value Scheduler::io_write(IOHandle* handle, const Value &val) {
    if (!handle) {
        throw std::runtime_error("Invalid I/O handler for write operation");
    }

    if (handle->closed) {
        throw std::runtime_error("Cannot write to a closed I/O handler");
    }

    if (val.is_string()) {
        const std::string &str = val.as_string();
        handle->write_buffer.assign(str.begin(), str.end());
    } else if (val.is_byte_array()) {
        ByteArray* byte_arr = val.as_byte_array();
        handle->write_buffer.assign(byte_arr->data.begin(), byte_arr->data.end());
    } else {
        throw std::runtime_error("Unsupported value type for write operation");
    }

    size_t total_bytes = handle->write_buffer.size();

    while (!handle->write_buffer.empty()) {
        ssize_t bytes_written = write(handle->fd, handle->write_buffer.data(), handle->write_buffer.size());
        if (bytes_written == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to write to I/O handler: " + std::string(strerror(errno)));
            }

            handle->write_interest = true;

            IOOperation pending_write;
            pending_write.type = IOOperation::Type::Write;
            pending_write.thread = vm.current_thread;
            pending_write.nbytes = total_bytes;
            handle->writes.push_back(pending_write);

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
            return 0;
        }

        // Remove the bytes that were successfully written from the buffer
        handle->write_buffer.erase(handle->write_buffer.begin(), handle->write_buffer.begin() + bytes_written);
    }

    return static_cast<int>(total_bytes);
}

Value Scheduler::io_read_num_bytes(IOHandle* handle, size_t nbytes) {
    if (!handle) {
        throw std::runtime_error("Invalid I/O handler for read operation");
    }

    if (handle->closed) {
        throw std::runtime_error("Cannot read from a closed I/O handler");
    }

    if (nbytes == 0) return vm.heap->allocate<ByteArray>(); // Return an empty byte array if 0 bytes requested

    while (handle->read_buffer.size() < nbytes) {
        // Not enough data in the buffer, try reading more from the file descriptor
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_read = read(handle->fd, chunk, CHUNK_SIZE);
        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read from I/O handler: " + std::string(strerror(errno)));
            }

            IOOperation pending_read;
            pending_read.type = IOOperation::Type::ReadNumBytes;
            pending_read.thread = vm.current_thread;
            pending_read.nbytes = nbytes;
            handle->reads.push_back(pending_read);

            handle->read_interest = true;

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
            return {};
        }

        if (bytes_read == 0) {
            // EOF reached, return whatever is left in the buffer (if anything)
            std::vector<uint8_t> remaining(handle->read_buffer.begin(), handle->read_buffer.end());
            handle->read_buffer.clear();
            auto result = vm.heap->allocate<ByteArray>(std::move(remaining));
            return result;
        }

        // Append the newly read data to the handle's read buffer
        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_read);
    }

    // We have enough data in the buffer to fulfill the read request
    std::vector<uint8_t> result(handle->read_buffer.begin(), handle->read_buffer.begin() + nbytes);
    handle->read_buffer.erase(handle->read_buffer.begin(), handle->read_buffer.begin() + nbytes); // Remove the returned data from the buffer
    auto out = vm.heap->allocate<ByteArray>(std::move(result));
    return out;
}

Value Scheduler::io_read_until_delimiter(IOHandle* handle, uint8_t delimiter) {
    if (!handle) {
        throw std::runtime_error("Invalid I/O handler for read operation");
    }

    if (handle->closed) {
        throw std::runtime_error("Cannot read from a closed I/O handler");
    }

    auto it = std::find(handle->read_buffer.begin(), handle->read_buffer.end(), delimiter);
    while (it == handle->read_buffer.end()) {
        // Delimiter not found, try reading more from the file descriptor
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_read = read(handle->fd, chunk, CHUNK_SIZE);

        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read from I/O handler: " + std::string(strerror(errno)));
            }

            IOOperation pending_read;
            pending_read.type = IOOperation::Type::ReadUntilDelimiter;
            pending_read.thread = vm.current_thread;
            pending_read.delimiter = delimiter;
            handle->reads.push_back(pending_read);

            handle->read_interest = true;

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
            return {};
        }

        if (bytes_read == 0) {
            // EOF reached, return whatever is left in the buffer (if anything)
            std::vector<uint8_t> remaining(handle->read_buffer.begin(), handle->read_buffer.end());
            handle->read_buffer.clear();
            auto result = vm.heap->allocate<ByteArray>(std::move(remaining));
            return result;
        }

        // Append new data to the buffer and check for the delimiter again
        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_read);

        // Check for the delimiter again after reading more data
        it = std::find(handle->read_buffer.end() - bytes_read, handle->read_buffer.end(), delimiter);
    }

    // Delimiter found, return the data up to and including the delimiter
    std::vector<uint8_t> line(handle->read_buffer.begin(), it + 1); // Include the delimiter
    handle->read_buffer.erase(handle->read_buffer.begin(), it + 1); // Remove the line from the buffer
    auto out = vm.heap->allocate<ByteArray>(std::move(line));
    return out;
}

Value Scheduler::io_read_all(IOHandle* handle) {
    if (!handle) {
        throw std::runtime_error("Invalid I/O handler for read operation");
    }

    if (handle->closed) {
        throw std::runtime_error("Cannot read from a closed I/O handler");
    }

    while (true) {
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_read = read(handle->fd, chunk, CHUNK_SIZE);
        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read from I/O handler: " + std::string(strerror(errno)));
            }

            IOOperation pending_read;
            pending_read.type = IOOperation::Type::ReadAll;
            pending_read.thread = vm.current_thread;
            handle->reads.push_back(pending_read);

            handle->read_interest = true;

            vm.current_thread->state = GreenThread::Blocked;
            vm.current_thread->wake_time = {};
            return {};
        }

        if (bytes_read == 0) break; // EOF reached

        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_read);
    }

        auto out = vm.heap->allocate<ByteArray>(std::move(handle->read_buffer));
        return out;
}

Value Scheduler::io_accept(IOHandle* handle) {
    if (!handle || (handle->kind != IOHandle::Kind::ListenerUnix && handle->kind != IOHandle::Kind::ListenerTCP)) {
        throw std::runtime_error("Invalid I/O handler for accept operation");
    }

    if (handle->closed) {
        throw std::runtime_error("Cannot accept on a closed I/O handler");
    }

    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(handle->fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
        }

        std::cerr << "[I/O accept would block on handle " << handle->fd << ", blocking thread " << vm.current_thread->ID << "]\n";

        IOOperation pending_accept;
        pending_accept.type = IOOperation::Type::Accept;
        pending_accept.thread = vm.current_thread;
        handle->accepts.push_back(pending_accept);

        handle->read_interest = true;

        vm.current_thread->state = GreenThread::Blocked;
        vm.current_thread->wake_time = {};
        return {};
    }

    return client_handle_from(client_fd, handle, client_addr);
}

Value Scheduler::client_handle_from(int client_fd, IOHandle* handle, sockaddr_storage &client_addr) {
    set_non_blocking(client_fd);

    auto client_handle = vm.heap->allocate<IOHandle>();
    client_handle->fd = client_fd;

    switch (handle->kind) {
        case IOHandle::Kind::ListenerUnix:
            client_handle->kind = IOHandle::Kind::StreamUnix;
            client_handle->metadata.path = handle->metadata.path;
            break;
        case IOHandle::Kind::ListenerTCP: {
            set_tcp_stream_options(client_fd);

            struct sockaddr_in *client_addr_in = (struct sockaddr_in *)&client_addr;
            char client_ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &client_addr_in->sin_addr, client_ip, sizeof(client_ip)) == nullptr) {
                throw std::runtime_error("Failed to convert client IP address: " + std::string(strerror(errno)));
            }

            uint16_t client_port = ntohs(client_addr_in->sin_port);

            client_handle->kind = IOHandle::Kind::StreamTCP;
            client_handle->metadata.local_address = handle->metadata.local_address;
            client_handle->metadata.remote_address = std::string(client_ip) + ":" + std::to_string(client_port);
            break;
        }
        default:
            throw std::runtime_error("Invalid listener type for accept operation");
    }

    io_poller->add_handle(client_handle);

    return client_handle;
}

void Scheduler::io_close(IOHandle* handle) {
    if (!handle) {
        throw std::runtime_error("Invalid I/O handler for close operation");
    }

    if (handle->closed) {
        return; // Already closed, no-op
    }

    close(handle->fd);
    handle->closed = true;
    handle->read_interest = false;
    handle->write_interest = false;

    // Notify any threads waiting on this handle that it's now closed
    for (auto &pending_read : handle->reads) {
        pending_read.thread->ctx.poke_stack({});
        pending_read.thread->state = GreenThread::Ready;
        enqueue(pending_read.thread);
    }
    handle->reads.clear();

    for (auto &pending_write : handle->writes) {
        pending_write.thread->ctx.poke_stack({});
        pending_write.thread->state = GreenThread::Ready;
        enqueue(pending_write.thread);
    }
    handle->writes.clear();

    for (auto &pending_accept : handle->accepts) {
        pending_accept.thread->ctx.poke_stack({});
        pending_accept.thread->state = GreenThread::Ready;
        enqueue(pending_accept.thread);
    }
    handle->accepts.clear();

    for (auto &pending_connect : handle->connects) {
        pending_connect.thread->ctx.poke_stack({});
        pending_connect.thread->state = GreenThread::Ready;
        enqueue(pending_connect.thread);
    }
    handle->connects.clear();
}

void Scheduler::complete_connects(IOHandle* handle) {
    while (!handle->connects.empty()) {
        auto &pending_connect = handle->connects.front();

        int err;
        socklen_t err_len = sizeof(err);
        if (getsockopt(handle->fd, SOL_SOCKET, SO_ERROR, &err, &err_len) == -1) {
            throw std::runtime_error("Failed to get socket error status: " + std::string(strerror(errno)));
        }

        if (err == 0) {
            // Connection successful
            if (handle->kind == IOHandle::Kind::StreamTCP) {
                struct sockaddr_in local_addr;
                socklen_t local_addr_len = sizeof(local_addr);
                if (getsockname(handle->fd, (struct sockaddr*)&local_addr, &local_addr_len) == -1) {
                    throw std::runtime_error("Failed to get local socket address: " + std::string(strerror(errno)));
                }

                char local_ip[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip)) == nullptr) {
                    throw std::runtime_error("Failed to convert local IP address: " + std::string(strerror(errno)));
                }

                uint16_t local_port = ntohs(local_addr.sin_port);
                handle->metadata.local_address = std::string(local_ip) + ":" + std::to_string(local_port);
            }

            pending_connect.thread->ctx.poke_stack(handle);
            pending_connect.thread->state = GreenThread::Ready;
            enqueue(pending_connect.thread);
        } else {
            // Connection failed
            pending_connect.thread->ctx.poke_stack({});
            pending_connect.thread->state = GreenThread::Ready;
            enqueue(pending_connect.thread);
        }

        handle->connects.pop_front();
    }
}

void Scheduler::complete_reads(IOHandle* handle) {
    std::cerr << "[Completing reads for socket " << handle->fd << "]\n";
    while (!handle->reads.empty()) {
        const auto &pending_read = handle->reads.front();
        handle->reads.pop_front();

        switch (pending_read.type) {
            case IOOperation::Type::ReadNumBytes:
                complete_read_num_bytes(handle, pending_read);
                break;
            case IOOperation::Type::ReadUntilDelimiter:
                complete_read_until_delimiter(handle, pending_read);
                break;
            case IOOperation::Type::ReadAll:
                complete_read_all(handle, pending_read);
                break;
            default:
                throw std::runtime_error("Invalid read operation type");
        }

        if (pending_read.thread->state == GreenThread::Blocked) {
            handle->reads.push_front(pending_read); // Put it back to the front of the queue
            return; // still not ready, stay blocked
        }
    }

    if (handle->reads.empty()) {
        handle->read_interest = false;
    }
}

void Scheduler::complete_read_num_bytes(IOHandle* handle, const IOOperation &pending_read) {
    while (handle->read_buffer.size() < pending_read.nbytes) {
        // Not enough data in the buffer, try reading more from the file descriptor
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_received = read(handle->fd, chunk, CHUNK_SIZE);
        if (bytes_received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read data from stream: " + std::string(strerror(errno)));
            }
            return; // still not ready, stay blocked
        }

        if (bytes_received == 0) {
            // EOF reached, return whatever we have in the buffer (if anything)
            std::vector<uint8_t> result = handle->read_buffer;
            handle->read_buffer.clear();
            pending_read.thread->ctx.poke_stack(vm.heap->allocate<ByteArray>(result));
            pending_read.thread->state = GreenThread::Ready;
            enqueue(pending_read.thread);
            return;
        }

        // Append the newly read data to the handle's read buffer
        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_received);
    }

    // We have enough data in the buffer to fulfill the read request
    std::vector<uint8_t> result(handle->read_buffer.begin(), handle->read_buffer.begin() + pending_read.nbytes);
    handle->read_buffer.erase(handle->read_buffer.begin(), handle->read_buffer.begin() + pending_read.nbytes); // Remove the returned data from the buffer
    pending_read.thread->ctx.poke_stack(vm.heap->allocate<ByteArray>(std::move(result)));
    pending_read.thread->state = GreenThread::Ready;
    enqueue(pending_read.thread);
}

void Scheduler::complete_read_until_delimiter(IOHandle* handle, const IOOperation &pending_read) {
    std::cerr << "[Completing read until delimiter for socket " << handle->fd << "]\n";
    uint8_t delimiter = pending_read.delimiter;

    auto it = std::find(handle->read_buffer.begin(), handle->read_buffer.end(), delimiter);
    while (it == handle->read_buffer.end()) {
        // Delimiter not found, try reading more from the file descriptor
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_received = read(handle->fd, chunk, CHUNK_SIZE);
        if (bytes_received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read data from stream: " + std::string(strerror(errno)));
            }
            return; // still not ready, stay blocked
        }

        if (bytes_received == 0) {
            // EOF reached, return whatever we have
            std::vector<uint8_t> result = handle->read_buffer;
            handle->read_buffer.clear();
            pending_read.thread->ctx.poke_stack(vm.heap->allocate<ByteArray>(result));
            pending_read.thread->state = GreenThread::Ready;
            enqueue(pending_read.thread);
            return;
        }

        // Append new data to the buffer and check for the delimiter again
        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_received);

        // Check for the delimiter again after reading more data
        it = std::find(handle->read_buffer.end() - bytes_received, handle->read_buffer.end(), delimiter);
    }

    // Delimiter found in buffer, return data up to the delimiter and remove it from the buffer
    std::vector<uint8_t> line(handle->read_buffer.begin(), it + 1); // Include the delimiter
    handle->read_buffer.erase(handle->read_buffer.begin(), it + 1); // Remove the line from the buffer
    pending_read.thread->ctx.poke_stack(vm.heap->allocate<ByteArray>(line));
    pending_read.thread->state = GreenThread::Ready;
    enqueue(pending_read.thread);
}

void Scheduler::complete_read_all(IOHandle* handle, const IOOperation &pending_read) {
    while (true) {
        uint8_t chunk[CHUNK_SIZE];
        ssize_t bytes_received = read(handle->fd, chunk, CHUNK_SIZE);

        if (bytes_received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to read data from stream: " + std::string(strerror(errno)));
            }
            return; // still not ready, stay blocked
        }

        if (bytes_received == 0) {
            // EOF reached
            std::vector<uint8_t> result = handle->read_buffer;
            handle->read_buffer.clear();
            pending_read.thread->ctx.poke_stack(vm.heap->allocate<ByteArray>(result));
            pending_read.thread->state = GreenThread::Ready;
            enqueue(pending_read.thread);
            return;
        }

        handle->read_buffer.insert(handle->read_buffer.end(), chunk, chunk + bytes_received);
    }
}

void Scheduler::complete_writes(IOHandle* handle) {
    while (!handle->write_buffer.empty() && !handle->writes.empty()) {
        auto &pending_write = handle->writes.front();

        complete_write(handle, pending_write);

        if (pending_write.thread->state == GreenThread::Blocked) {
            return; // still not ready, stay blocked
        }

        handle->writes.pop_front();
    }

    if (handle->write_buffer.empty()) {
        handle->write_interest = false;
    }
}

void Scheduler::complete_write(IOHandle* handle, const IOOperation &pending_write) {
    while (!handle->write_buffer.empty()) {
        ssize_t bytes_sent = write(handle->fd, handle->write_buffer.data(), handle->write_buffer.size());
        if (bytes_sent < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to write data on stream: " + std::string(strerror(errno)));
            }
            return; // still not ready, stay blocked
        }

        // Remove sent data from buffer
        handle->write_buffer.erase(handle->write_buffer.begin(), handle->write_buffer.begin() + bytes_sent);
    }

    pending_write.thread->ctx.poke_stack(static_cast<int>(pending_write.nbytes));
    pending_write.thread->state = GreenThread::Ready;
    enqueue(pending_write.thread);
}

void Scheduler::complete_accepts(IOHandle* handle) {
    while (!handle->accepts.empty()) {
        auto &pending_accept = handle->accepts.front();

        complete_accept(handle, pending_accept);

        if (pending_accept.thread->state == GreenThread::Blocked) {
            return; // still not ready, stay blocked
        }

        handle->accepts.pop_front();
    }

    if (handle->accepts.empty()) {
        handle->read_interest = false;
    }
}

void Scheduler::complete_accept(IOHandle* handle, const IOOperation &pending_accept) {
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(handle->fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
        }
        return; // still not ready, stay blocked
    }

    std::cerr << "[Accepted new connection on socket " << handle->fd << " with fd " << client_fd << "]\n";

    auto client_handle = client_handle_from(client_fd, handle, client_addr);

    pending_accept.thread->ctx.poke_stack(client_handle);
    pending_accept.thread->state = GreenThread::Ready;
    enqueue(pending_accept.thread);
}

void Scheduler::poll_io_events(int timeout_ms) {
    io_poller->poll(timeout_ms, [this](const IOEvent& event) {
        if (event.type == IOEvent::Type::Readable && event.handle->read_interest) {
            switch (event.handle->kind) {
                case IOHandle::Kind::ListenerUnix:
                case IOHandle::Kind::ListenerTCP:
                    complete_accepts(event.handle);
                    break;
                default:
                    complete_reads(event.handle);
                    break;
            };
        }

        if (event.type == IOEvent::Type::Writable && event.handle->write_interest) {
            complete_connects(event.handle);
            complete_writes(event.handle);
        }
    });
}

void Scheduler::handle_thread_state(GreenThread* &next_thread) {
    if (next_thread->state == GreenThread::Finished) {
        notify_waiters(next_thread);
        finish_children(next_thread);
        return;
    }

    if (next_thread->state == GreenThread::Blocked) {
        if (next_thread->wake_time == std::chrono::steady_clock::time_point{}) {
            // Blocked without a wake time (e.g., waiting for join, pipe I/O, select)
            return;
        }

        block_thread(next_thread);
        return;
    }

    next_thread->state = GreenThread::Ready;
    enqueue(next_thread);
}

Value Scheduler::schedule() {
    while (vm.main_thread->state != GreenThread::Finished) {
        if (vm_signal::suspend_requested) break;

        std::cerr << "[Scheduler loop iteration]\n";

        auto now = std::chrono::steady_clock::now();
        wake_threads(now);

        // If a thread is already runnable, dispatch it immediately instead of
        // waiting in epoll and delaying select/pipe wakeups.
        if (ready_queue.empty()) {
            // Calculate how long to wait for socket events
            // (until the next thread wake time, or a default timeout)
            int poll_timeout_ms = calculate_poll_timeout(now);
            poll_io_events(poll_timeout_ms);
        }

        auto next_thread = dequeue();
        if (!next_thread) continue;

        std::cerr << "[Scheduling thread " << next_thread->ID << "]\n";

        next_thread->state = GreenThread::Running;

        vm.current_thread = next_thread;
        vm.run();
        vm.current_thread = nullptr;

        handle_thread_state(next_thread);
    }

    return vm.main_thread->return_value;
}
