#include "deserializer.hpp"
#include "green_thread.hpp"
#include "io_handler.hpp"
#include "pipe.hpp"
#include "scheduler.hpp"
#include "vm.hpp"
#include "runtime.hpp"
#include "value.hpp"
#include <iostream>

std::pair<std::string, uint16_t> parse_host_port(const std::string &endpoint) {
    const size_t sep = endpoint.rfind(':');
    if (sep == std::string::npos) {
        throw std::runtime_error("Invalid TCP endpoint '" + endpoint + "'");
    }

    std::string host = endpoint.substr(0, sep);
    int port = std::stoi(endpoint.substr(sep + 1));
    if (port < 0 || port > 65535) {
        throw std::runtime_error("Invalid TCP endpoint port in '" + endpoint + "'");
    }

    return {std::move(host), static_cast<uint16_t>(port)};
};

void BinaryDeserializer::read_vm(VM& vm) {
    constexpr std::string_view expected_magic = "dog:vm-state";
    std::string magic(expected_magic.size(), '\0');
    read_buffer(magic.data(), magic.size());

    if (magic != expected_magic) {
        throw std::runtime_error("Invalid VM state file. Got magic word \"" + magic + "\"");
    }

    id_size = read_number<uint8_t>();

    size_t free_id_count = read_number<size_t>();
    for (size_t i = 0; i < free_id_count; i++) {
        vm.heap->free_ids.push_back(read_number<size_t>());
    }

    //----------------------------------------------------------------------
    // Heap objects
    //----------------------------------------------------------------------

    size_t object_count = read_number<size_t>();

    this->vm = &vm;
    vm.heap->active_vm = nullptr;
    vm.heap->objects.clear();
    vm.heap->next_id = 1;

    for (size_t i = 0; i < object_count; i++) {
        size_t id = read_id();
        uint8_t tag = read_number<uint8_t>();

        // std::cerr << "id = " << id << ", tag = " << (int)tag << "\n";

        Object* obj;

        switch (tag) {
            case 1:  obj = vm.heap->allocate_empty<Function>(); break;
            case 2:  obj = vm.heap->allocate_empty<Native>(); break;
            case 3:  obj = vm.heap->allocate_empty<Closure>(); break;
            case 4:  obj = vm.heap->allocate_empty<MethodClosure>(); break;
            case 5:  obj = vm.heap->allocate_empty<Upvalue>(); break;
            case 6:  obj = vm.heap->allocate_empty<Array>(); break;
            case 7:  obj = vm.heap->allocate_empty<Record>(); break;
            case 8:  obj = vm.heap->allocate_empty<ByteArray>(); break;
            case 9:  obj = vm.heap->allocate_empty<Struct>(); break;
            case 10: obj = vm.heap->allocate_empty<StructInstance>(); break;
            case 11: obj = vm.heap->allocate_empty<GreenThread>(); break;
            case 12: obj = vm.heap->allocate_empty<IOHandle>(); break;
            case 13: obj = vm.heap->allocate_empty<Pipe>(); break;
            default:
                throw std::runtime_error(std::string("Unknown object tag") + std::to_string(tag));
        }

        obj->id = id;

        if (id >= vm.heap->next_id)
            vm.heap->next_id = id + 1;

        id_map[id] = obj;
    }

    for (size_t i = 0; i < object_count; i++) {
        size_t id = read_id();
        Object* obj = nullptr;

        try {
            obj = id_map.at(id);
        } catch (std::exception e) {
            // std::cerr << "Error with id " << id << ", " << e.what() << "\n";
            exit(1);
        }

        // std::cerr << "Obj with id = " << id << " : " << obj << "\n";
        obj->deserialize(*this);
    }

    //----------------------------------------------------------------------
    // Globals
    //----------------------------------------------------------------------

    vm.globals.clear();

    size_t global_count = read_number<size_t>();

    for (size_t i = 0; i < global_count; i++) {
        std::string key = read_string();
        Value value = read_value();
        vm.globals.emplace(std::move(key), std::move(value));
    }

    //----------------------------------------------------------------------
    // Main thread
    //----------------------------------------------------------------------

    if (read_number<bool>()) {
        size_t id = read_id();
        vm.main_thread = dynamic_cast<GreenThread*>(id_map.at(id)); // MUST BE RESOLVED
    } else {
        vm.main_thread = nullptr;
    }

    //----------------------------------------------------------------------
    // Current thread
    //----------------------------------------------------------------------

    if (read_number<bool>()) {
        size_t id = read_id();
        vm.current_thread = dynamic_cast<GreenThread*>(id_map.at(id)); // MUST BE RESOLVED
    } else {
        vm.current_thread = nullptr;
    }

    //----------------------------------------------------------------------
    // CLI arguments
    //----------------------------------------------------------------------

    vm.cli_arguments = read_value();

    //----------------------------------------------------------------------
    // Scheduler
    //----------------------------------------------------------------------

    vm.scheduler.next_thread_id = read_number<size_t>();
    vm.scheduler.next_pipe_id   = read_number<size_t>();

    //----------------------------------------------------------------------
    // Ready queue
    //----------------------------------------------------------------------

    vm.scheduler.ready_queue.clear();

    size_t ready_count = read_number<size_t>();

    for (size_t i = 0; i < ready_count; i++) {

        size_t id = read_id();

        vm.scheduler.ready_queue.push_back(
            dynamic_cast<GreenThread*>(id_map.at(id))
        ); // MUST BE RESOLVED
    }

    //----------------------------------------------------------------------
    // Blocked queue
    //----------------------------------------------------------------------

    while (!vm.scheduler.blocked_queue.empty())
        vm.scheduler.blocked_queue.pop();

    size_t blocked_count = read_number<size_t>();

    for (size_t i = 0; i < blocked_count; i++) {

        size_t id = read_id();
        size_t ms = read_number<size_t>();

        auto wake_time = now + std::chrono::milliseconds(ms);

        vm.scheduler.blocked_queue.emplace(
            wake_time,
            dynamic_cast<GreenThread*>(id_map.at(id))
        ); // MUST BE RESOLVED
    }

    //----------------------------------------------------------------------
    // Synchronize stream state with found listeners
    // For each stream, if it has a local address, find a matching listener and synchronize state
    //----------------------------------------------------------------------
    // for (auto stream : found_streams) {
    //     // If the stream has a local address, find a matching listener
    //     if (stream->kind == IOHandle::Kind::StreamTCP && !stream->metadata.local_address.empty()) {
    //         bool found = false;
    //         for (auto listener : found_listeners) {
    //             if (stream->metadata.local_address == listener->metadata.local_address) {
    //                 found = true;
    //                 break;
    //             }
    //         }

    //         if (!found) {
    //             // Stream has a local address but no matching listener found, has to connect to a remote address
    //             auto [host, port] = parse_host_port(stream->metadata.remote_address);

    //             int fd = socket(AF_INET, SOCK_STREAM, 0);
    //             if (fd == -1) {
    //                 throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    //             }

    //             set_tcp_stream_options(fd);

    //             struct sockaddr_in remote_addr = {};
    //             remote_addr.sin_family = AF_INET;
    //             remote_addr.sin_port = htons(port);
    //             if (inet_pton(AF_INET, host.c_str(), &remote_addr.sin_addr) <= 0) {
    //                 close(fd);
    //                 throw std::runtime_error("Invalid IP address: " + std::string(strerror(errno)));
    //             }

    //             if (connect(fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) == -1) {
    //                 close(fd);
    //                 throw std::runtime_error("Failed to connect: " + std::string(strerror(errno)));
    //             }

    //             stream->fd = fd;
    //         }
    //     }
    // }

    // for (auto stream : found_streams) {
    //     // If the stream has a local address, find a matching listener
    //     if (stream->kind == IOHandle::Kind::StreamTCP && !stream->metadata.local_address.empty()) {
    //         for (auto listener : found_listeners) {
    //             if (stream->metadata.local_address == listener->metadata.local_address) {
    //                 // Listener has to accept connection
    //                 struct sockaddr_storage client_addr;
    //                 socklen_t client_addr_len = sizeof(client_addr);

    //                 set_blocking(listener->fd);

    //                 int client_fd = accept(listener->fd, (struct sockaddr*)&client_addr, &client_addr_len);
    //                 if (client_fd == -1) {
    //                     throw std::runtime_error("accept failed: " + std::string(strerror(errno)));
    //                 }

    //                 set_non_blocking(listener->fd);

    //                 stream->fd = client_fd;

    //                 set_tcp_stream_options(client_fd);

    //                 struct sockaddr_in *client_addr_in = (struct sockaddr_in *)&client_addr;
    //                 char client_ip[INET_ADDRSTRLEN];
    //                 if (inet_ntop(AF_INET, &client_addr_in->sin_addr, client_ip, sizeof(client_ip)) == nullptr) {
    //                     throw std::runtime_error("Failed to convert client IP address: " + std::string(strerror(errno)));
    //                 }

    //                 uint16_t client_port = ntohs(client_addr_in->sin_port);

    //                 stream->metadata.remote_address = std::string(client_ip) + ":" + std::to_string(client_port);

    //                 set_non_blocking(client_fd);

    //                 vm.scheduler.io_poller->add_handle(stream);
    //             }
    //         }
    //     }
    // }

    // for (auto stream : found_streams) {
    //     // If the stream has a local address, find a matching listener
    //     if (stream->kind == IOHandle::Kind::StreamTCP && !stream->metadata.local_address.empty()) {
    //         bool found = false;
    //         for (auto listener : found_listeners) {
    //             if (stream->metadata.local_address == listener->metadata.local_address) {
    //                 found = true;
    //                 break;
    //             }
    //         }

    //         if (!found) {
    //             int err;
    //             socklen_t err_len = sizeof(err);
    //             if (getsockopt(stream->fd, SOL_SOCKET, SO_ERROR, &err, &err_len) == -1) {
    //                 throw std::runtime_error("Failed to get socket error status: " + std::string(strerror(errno)));
    //             }

    //             if (err != 0) {
    //                 throw std::runtime_error("Socket error: " + std::string(strerror(err)));
    //             }

    //             // Get the local address and port assigned to the socket (useful for clients that bind to an ephemeral port)
    //             struct sockaddr_in local_addr;
    //             socklen_t local_addr_len = sizeof(local_addr);
    //             if (getsockname(stream->fd, (struct sockaddr*)&local_addr, &local_addr_len) == -1) {
    //                 close(stream->fd);
    //                 throw std::runtime_error("Failed to get local socket address: " + std::string(strerror(errno)));
    //             }

    //             char local_ip[INET_ADDRSTRLEN];
    //             if (inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, sizeof(local_ip)) == nullptr) {
    //                 close(stream->fd);
    //                 throw std::runtime_error("Failed to convert local IP address: " + std::string(strerror(errno)));
    //             }

    //             uint16_t local_port = ntohs(local_addr.sin_port);
    //             stream->metadata.local_address = std::string(local_ip) + ":" + std::to_string(local_port);

    //             set_non_blocking(stream->fd);

    //             vm.scheduler.io_poller->add_handle(stream);
    //         }
    //     }
    // }
}

Value BinaryDeserializer::read_value() {
    char tag;
    in.read(&tag, 1);

    switch (tag) {
        case 0: return {};
        case 1: return read_number<int>();
        case 2: return read_number<double>();
        case 3: return read_number<bool>();
        case 4: return read_string();
        case 5: {
            size_t id = read_id();
            return id_map.at(id);
        }
        default:
            throw std::runtime_error(std::string("Unknown value type tag: ") + tag);
    }
}

void BinaryDeserializer::read_function(Function* func) {
    func->name = read_string();
    func->arity = read_number<size_t>();
    func->upvalue_count = read_number<size_t>();

    // ----------------------------------------------------
    // Bytecode
    // ----------------------------------------------------

    size_t code_size = read_number<size_t>();
    func->chunk.code.resize(code_size);

    for (size_t i = 0; i < code_size; i++) {
        func->chunk.code[i] = read_number<uint8_t>();
    }

    // ----------------------------------------------------
    // Constants
    // ----------------------------------------------------

    size_t const_size = read_number<size_t>();
    func->chunk.constants.resize(const_size);

    for (size_t i = 0; i < const_size; i++) {
        func->chunk.constants[i] = read_value();
    }
}

void BinaryDeserializer::read_native(Native* native) {
    native->name = read_string();
    native->arity = static_cast<int>(read_number<size_t>());
    native->func = vm->resolve_native(native->name, native->arity);
    native->bound_instance = read_value();
}

void BinaryDeserializer::read_closure(Closure* closure) {
    size_t func_id = read_id();
    closure->func = static_cast<Function*>(id_map.at(func_id));

    size_t upvalue_count = read_number<size_t>();
    closure->upvalues.resize(upvalue_count);

    for (size_t i = 0; i < upvalue_count; i++) {
        closure->upvalues[i] = read_value().as_upvalue();
    }
}

void BinaryDeserializer::read_method_closure(MethodClosure* method_closure) {
    size_t closure_id = read_id();
    method_closure->closure = dynamic_cast<Closure*>(id_map.at(closure_id));
    method_closure->self = read_value();
}

void BinaryDeserializer::read_upvalue(Upvalue* upvalue) {
    // owner_thread (may be null)
    size_t thread_id = read_id();

    if (thread_id == 0) {
        upvalue->owner_thread = nullptr;
    } else {
        upvalue->owner_thread = dynamic_cast<GreenThread*>(id_map.at(thread_id));
    }

    upvalue->slot_index = read_number<int>();
    upvalue->closed = read_value();
}

void BinaryDeserializer::read_array(Array* array) {
    size_t array_size = read_number<size_t>();
    array->elements.resize(array_size);

    for (size_t i = 0; i < array_size; i++) {
        (*array)[i] = read_value();
    }
}

void BinaryDeserializer::read_record(Record* record) {
    size_t record_size = read_number<size_t>();
    record->items.reserve(record_size);

    for (size_t i = 0; i < record_size; i++) {
        std::string key = read_string();
        Value value = read_value();
        (*record)[key] = value;
    }
}

void BinaryDeserializer::read_byte_array(ByteArray* byte_array) {
    size_t nbytes = read_number<size_t>();
    byte_array->data.resize(nbytes);
    read_buffer(reinterpret_cast<char*>(byte_array->data.data()), nbytes);
}

void BinaryDeserializer::read_struct(Struct* strct) {
    std::string struct_name = read_string();
    strct->name = std::move(struct_name);

    size_t method_count = read_number<size_t>();
    strct->methods.reserve(method_count);

    for (size_t i = 0; i < method_count; i++) {
        std::string key = read_string();
        Value value = read_value();
        strct->methods[key] = value;
    }
}

void BinaryDeserializer::read_struct_instance(StructInstance* instance) {
    size_t instance_id = read_id();
    instance->struct_ptr = dynamic_cast<Struct*>(id_map.at(instance_id));

    size_t field_count = read_number<size_t>();
    instance->fields.reserve(field_count);

    for (size_t i = 0; i < field_count; i++) {
        std::string key = read_string();
        Value value = read_value();
        instance->fields[key] = value;
    }
}

void BinaryDeserializer::read_thread(GreenThread* thread) {
    thread->ID = read_id();
    thread->state = static_cast<GreenThread::State>(read_number<uint8_t>());

    auto ms = read_number<size_t>();
    if (ms == 0) {
        thread->wake_time = {};
    } else {
        thread->wake_time = now + std::chrono::milliseconds(ms);
    }

    thread->detached = read_number<bool>();
    thread->return_value = read_value();
    thread->pending_value = read_value();

    if (read_number<bool>()) {
        thread->parent = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }

    size_t joiner_count = read_number<size_t>();
    thread->joiners.resize(joiner_count);
    for (size_t i = 0; i < joiner_count; i++) {
        thread->joiners[i] = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }

    size_t children_count = read_number<size_t>();
    thread->children.resize(children_count);
    for (size_t i = 0; i < children_count; i++) {
        thread->children[i] = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }

    if (read_number<bool>()) {
        thread->active_select = std::make_unique<SelectFrame>();

        size_t case_count = read_number<size_t>();
        thread->active_select->cases.resize(case_count);

        for (size_t i = 0; i < case_count; i++) {
            auto &sel_case = thread->active_select->cases[i];

            sel_case.type = static_cast<SelectCase::Type>(read_number<uint8_t>());
            if (read_number<bool>()) {
                sel_case.pipe = dynamic_cast<Pipe*>(id_map.at(read_id()));
            }

            sel_case.target_ip = read_number<int>();
            if (sel_case.type == SelectCase::Recv) {
                sel_case.slot = read_number<uint8_t>();
            } else {
                sel_case.value = read_value();
            }
        }

        thread->active_select->has_default = read_number<bool>();
        if (thread->active_select->has_default) {
            thread->active_select->default_target_ip = read_number<int>();
        }
    }

    thread->ctx.stack_size = read_number<size_t>();
    for (size_t i = 0; i < thread->ctx.stack_size; i++) {
        thread->ctx.stack[i] = read_value();
    }

    size_t frame_count = read_number<size_t>();
    thread->ctx.frames.resize(frame_count);
    for (size_t i = 0; i < frame_count; i++) {
        auto& frame = thread->ctx.frames[i];
        frame.closure = dynamic_cast<Closure*>(id_map.at(read_id()));
        frame.base = read_number<int>();
        frame.ip = read_number<int>();
    }

    size_t upvalue_count = read_number<size_t>();
    thread->ctx.open_upvalues.resize(upvalue_count);
    for (size_t i = 0; i < upvalue_count; i++) {
        thread->ctx.open_upvalues[i] = dynamic_cast<Upvalue*>(id_map.at(read_id()));
    }
}

void BinaryDeserializer::read_io_handle(IOHandle* handle) {
    const IOHandle::Kind kind = static_cast<IOHandle::Kind>(read_number<uint8_t>());
    const bool closed = read_number<bool>();
    const bool read_interest = read_number<bool>();
    const bool write_interest = read_number<bool>();

    const std::string path = read_string();
    const std::string mode = read_string();
    const off_t offset = read_number<off_t>();
    const std::string local_address = read_string();
    const std::string remote_address = read_string();

    IOHandle* recreated = nullptr;
    switch (kind) {
        case IOHandle::Kind::Stdin:
            recreated = vm->scheduler.stdin_handle();
            break;
        case IOHandle::Kind::Stdout:
            recreated = vm->scheduler.stdout_handle();
            break;
        case IOHandle::Kind::File:
            recreated = vm->scheduler.file_open(path, (mode == "a" ? "w" : mode == "ra" ? "rw" : mode), offset);
            break;
        case IOHandle::Kind::ListenerUnix:
            recreated = vm->scheduler.socket_unix_listener(path);
            break;
        case IOHandle::Kind::ListenerTCP: {
            auto [address, port] = parse_host_port(local_address);
            recreated = vm->scheduler.socket_tcp_listener(address, port);
            break;
        }
        case IOHandle::Kind::StreamUnix:
        case IOHandle::Kind::StreamTCP:
            break;
    }

    if (recreated != nullptr) {
        struct stat st;
        fstat(recreated->fd, &st);

        if (!S_ISREG(st.st_mode)) {
            vm->scheduler.io_poller->remove_handle(recreated);
        }

        handle->fd = recreated->fd;

        auto it = std::find(vm->heap->objects.begin(), vm->heap->objects.end(), recreated);
        if (it != vm->heap->objects.end()) {
            vm->heap->total_bytes -= recreated->tracked_size;
            vm->heap->objects.erase(it);
        }
        delete recreated;

        if (!S_ISREG(st.st_mode)) {
            vm->scheduler.io_poller->add_handle(handle);
        }
    } else {
        handle->fd = -1;
    }

    handle->kind = kind;
    handle->closed = closed;
    handle->read_interest = read_interest;
    handle->write_interest = write_interest;

    handle->metadata.path = path;
    handle->metadata.mode = mode;
    handle->metadata.local_address = local_address;
    handle->metadata.remote_address = remote_address;

    // if (kind == IOHandle::Kind::ListenerTCP) {
    //     found_listeners.push_back(handle);
    // } else if (kind == IOHandle::Kind::StreamTCP) {
    //     found_streams.push_back(handle);
    // }

    auto read_ops = [&](std::deque<IOOperation> &ops) {
        size_t ops_count = read_number<size_t>();
        ops.resize(ops_count);

        for (size_t i = 0; i < ops_count; i++) {
            auto &op = ops[i];
            op.type = static_cast<IOOperation::Type>(read_number<uint8_t>());
            if (read_number<bool>()) {
                op.thread = dynamic_cast<GreenThread*>(id_map.at(read_id()));
            }

            switch (op.type) {
                case IOOperation::Type::ReadNumBytes:
                case IOOperation::Type::Write:
                    op.nbytes = read_number<size_t>();
                    break;
                case IOOperation::Type::ReadUntilDelimiter:
                    op.delimiter = read_number<uint8_t>();
                    break;
                case IOOperation::Type::ReadAll:
                case IOOperation::Type::Connect:
                case IOOperation::Type::Accept:
                    break;
            }
        }
    };

    read_ops(handle->reads);
    read_ops(handle->writes);
    read_ops(handle->accepts);
    read_ops(handle->connects);

    size_t write_buffer_size = read_number<size_t>();
    handle->write_buffer.resize(write_buffer_size);
    if (write_buffer_size != 0) {
        read_buffer(reinterpret_cast<char*>(handle->write_buffer.data()), write_buffer_size);
    }

    size_t read_buffer_size = read_number<size_t>();
    handle->read_buffer.resize(read_buffer_size);
    if (read_buffer_size != 0) {
        read_buffer(reinterpret_cast<char*>(handle->read_buffer.data()), read_buffer_size);
    }

    handle->eof_reached = read_number<bool>();
}

void BinaryDeserializer::read_pipe(Pipe* pipe) {
    pipe->ID = read_id();
    pipe->capacity = read_number<size_t>();
    pipe->closed = read_number<bool>();

    size_t buffer_size = read_number<size_t>();
    pipe->buffer.resize(buffer_size);
    for (size_t i = 0; i < buffer_size; i++) {
        pipe->buffer[i] = read_value();
    }

    size_t readers_size = read_number<size_t>();
    pipe->readers.resize(readers_size);
    for (size_t i = 0; i < readers_size; i++) {
        pipe->readers[i] = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }

    size_t writers_size = read_number<size_t>();
    pipe->writers.resize(writers_size);
    for (size_t i = 0; i < writers_size; i++) {
        pipe->writers[i] = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }

    size_t selectors_size = read_number<size_t>();
    pipe->selectors.resize(selectors_size);
    for (size_t i = 0; i < selectors_size; i++) {
        pipe->selectors[i] = dynamic_cast<GreenThread*>(id_map.at(read_id()));
    }
}
