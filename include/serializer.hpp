#pragma once

#include "common.hpp"

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

struct VM;

struct Serializer {
    virtual void print_vm(const VM &vm) = 0;

    virtual void print_value(const Value &value) = 0;

    virtual void print_function(Function* func) = 0;
    virtual void print_native(Native* native) = 0;
    virtual void print_closure(Closure* closure) = 0;
    virtual void print_method_closure(MethodClosure* method_closure) = 0;
    virtual void print_upvalue(Upvalue* upvalue) = 0;
    virtual void print_array(Array* array) = 0;
    virtual void print_record(Record* record) = 0;
    virtual void print_byte_array(ByteArray* byte_array) = 0;
    virtual void print_struct(Struct* strct) = 0;
    virtual void print_struct_instance(StructInstance* instance) = 0;
    virtual void print_thread(GreenThread* thread) = 0;
    virtual void print_io_handle(IOHandle* handle) = 0;
    virtual void print_pipe(Pipe* pipe) = 0;
};

struct JsonSerializer : public Serializer {
    std::ostream &out;

    std::chrono::steady_clock::time_point now;

    struct IndentGuard {
        size_t &level;
        IndentGuard(size_t &level) : level(level) { level++; }
        ~IndentGuard() { level--; }
    };

    size_t indent_level = 0;

    JsonSerializer(std::ostream &output) :
        out(output),
        now(std::chrono::steady_clock::now()) {}

    std::string indent(size_t nspaces = 2) {
        std::string result;
        for (size_t i = 0; i < indent_level * nspaces; i++) {
            result += " ";
        }

        return result;
    }

    void print_vm(const VM &vm) override;

    void print_value(const Value &value) override;

    void print_function(Function* func) override;
    void print_native(Native* native) override;
    void print_closure(Closure* closure) override;
    void print_method_closure(MethodClosure* method_closure) override;
    void print_upvalue(Upvalue* upvalue) override;
    void print_array(Array* array) override;
    void print_record(Record* record) override;
    void print_byte_array(ByteArray* byte_array) override;
    void print_struct(Struct* strct) override;
    void print_struct_instance(StructInstance* instance) override;
    void print_thread(GreenThread* thread) override;
    void print_io_handle(IOHandle* handle) override;
    void print_pipe(Pipe* pipe) override;
};

struct BinarySerializer : public Serializer {
    std::ostream &out;
    uint8_t id_size = 0;

    std::chrono::steady_clock::time_point now;

    BinarySerializer(std::ostream &output) :
        out(output),
        now(std::chrono::steady_clock::now()) {}

    template<typename T>
    inline void print_number(T num) { out.write(reinterpret_cast<const char*>(&num), sizeof(num)); }
    inline void print_id(size_t id) { out.write(reinterpret_cast<const char*>(&id), id_size); }

    inline void print_buffer(const char* buf, size_t n) { out.write(buf, n); }

    inline void print_string(const std::string& str) {
        print_number<size_t>(str.size());
        print_buffer(str.c_str(), str.size());
    }

    void print_vm(const VM &vm) override;

    void print_value(const Value &value) override;

    void print_function(Function* func) override;
    void print_native(Native* native) override;
    void print_closure(Closure* closure) override;
    void print_method_closure(MethodClosure* method_closure) override;
    void print_upvalue(Upvalue* upvalue) override;
    void print_array(Array* array) override;
    void print_record(Record* record) override;
    void print_byte_array(ByteArray* byte_array) override;
    void print_struct(Struct* strct) override;
    void print_struct_instance(StructInstance* instance) override;
    void print_thread(GreenThread* thread) override;
    void print_io_handle(IOHandle* handle) override;
    void print_pipe(Pipe* pipe) override;
};
