#pragma once

#include "memory.hpp"
#include "green_thread.hpp"
#include "value.hpp"

struct IOOperation {
    enum Type {
        ReadNumBytes,
        ReadUntilDelimiter,
        ReadAll,
        Write,
        Connect,
        Accept,
    } type;

    GreenThread* thread; // the thread that initiated this I/O operation
    size_t nbytes; // for ReadNumBytes and Write operations
    uint8_t delimiter; // for ReadUntilDelimiter operations
};

struct IOHandle : public Object {
    enum class Kind {
        ListenerUnix,
        ListenerTCP,
        StreamUnix,
        StreamTCP,
        File,
        Stdin,
    } kind;

    int fd;
    bool closed = false;

    bool read_interest = false;  // whether any thread is currently blocked on reading from this handle
    bool write_interest = false; // whether any thread is currently blocked on writing to this handle

    struct Metadata {
        std::string path;           // for file streams and UNIX socket listeners/streams
        std::string local_address;  // for TCP listeners/streams
        std::string remote_address; // for TCP streams
    } metadata;

    // Queues of pending I/O operations for this handle
    std::deque<IOOperation> reads;
    std::deque<IOOperation> writes;
    std::deque<IOOperation> accepts;  // for listeners
    std::deque<IOOperation> connects; // for streams in the process of connecting

    // Buffers for partial reads/writes
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;

    IOHandle() : Object(Type::IOHandle) {}

    inline std::string kind_name() const {
        switch (kind) {
            case Kind::ListenerUnix: return "ListenerUnix";
            case Kind::ListenerTCP:  return "ListenerTCP";
            case Kind::StreamUnix:   return "StreamUnix";
            case Kind::StreamTCP:    return "StreamTCP";
            case Kind::File:         return "File";
            case Kind::Stdin:        return "Stdin";
            default:                 return "Unknown";
        }
    }

    std::string type_name() const override { return "IOHandle"; }
    std::string to_string() const override { return "<IOHandle fd=" + std::to_string(fd) + ", kind=" + kind_name() + ">"; }
    bool is_truthy() const override { return !closed; }
    void serialize(Serializer& serializer) override { serializer.print_io_handle(this); }
    size_t object_size() const override {
        size_t size = sizeof(IOHandle);
        size += metadata.path.capacity() * sizeof(char);
        size += metadata.local_address.capacity() * sizeof(char);
        size += metadata.remote_address.capacity() * sizeof(char);
        size += read_buffer.capacity() * sizeof(uint8_t);
        size += write_buffer.capacity() * sizeof(uint8_t);
        size += reads.size() * sizeof(IOOperation);
        size += writes.size() * sizeof(IOOperation);
        size += accepts.size() * sizeof(IOOperation);
        size += connects.size() * sizeof(IOOperation);
        return size;
    }
};
