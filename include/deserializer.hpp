#pragma once

#include "common.hpp"
#include <vector>

struct Function;
struct Native;
struct Closure;
struct MethodClosure;
struct Array;
struct Record;
struct ByteArray;
struct Struct;
struct StructInstance;
struct Upvalue;
struct GreenThread;
struct Pipe;
struct IOHandle;

struct Value;

struct Object;
struct Heap;
struct VM;

struct Deserializer {
    virtual void read_vm(VM &vm) = 0;

    virtual Value read_value() = 0;

    virtual void read_function(Function* func) = 0;
    virtual void read_native(Native* native) = 0;
    virtual void read_closure(Closure* closure) = 0;
    virtual void read_method_closure(MethodClosure* method_closure) = 0;
    virtual void read_upvalue(Upvalue* upvalue) = 0;
    virtual void read_array(Array* array) = 0;
    virtual void read_record(Record* record) = 0;
    virtual void read_byte_array(ByteArray* byte_array) = 0;
    virtual void read_struct(Struct* strct) = 0;
    virtual void read_struct_instance(StructInstance* instance) = 0;
    virtual void read_thread(GreenThread* thread) = 0;
    virtual void read_io_handle(IOHandle* handle) = 0;
    virtual void read_pipe(Pipe* pipe) = 0;
};

struct BinaryDeserializer : public Deserializer {
    std::istream &in;
    uint8_t id_size = 0;
    std::chrono::steady_clock::time_point now;
    std::unordered_map<size_t, Object*> id_map;
    VM* vm = nullptr;

    std::vector<IOHandle*> found_listeners;
    std::vector<IOHandle*> found_streams;

    BinaryDeserializer(std::istream &in) :
        in(in),
        now(std::chrono::steady_clock::now()) {}

    template<typename T>
    inline T read_number() { T num; in.read(reinterpret_cast<char*>(&num), sizeof(num)); return num; }
    inline size_t read_id() {
        size_t id = 0;
        in.read(reinterpret_cast<char*>(&id), id_size);
        return id;
    }

    inline void read_buffer(char* buf, size_t n) { in.read(buf, n); }

    inline std::string read_string() {
        size_t str_size = read_number<size_t>();
        std::string str(str_size, 0);
        read_buffer(str.data(), str_size);
        return str;
    }

    void read_vm(VM &vm) override;

    Value read_value() override;

    void read_function(Function* func) override;
    void read_native(Native* native) override;
    void read_closure(Closure* closure) override;
    void read_method_closure(MethodClosure* method_closure) override;
    void read_upvalue(Upvalue* upvalue) override;
    void read_array(Array* array) override;
    void read_record(Record* record) override;
    void read_byte_array(ByteArray* byte_array) override;
    void read_struct(Struct* strct) override;
    void read_struct_instance(StructInstance* instance) override;
    void read_thread(GreenThread* thread) override;
    void read_io_handle(IOHandle* handle) override;
    void read_pipe(Pipe* pipe) override;
};
