#pragma once

#include "green_thread.hpp"
#include "value.hpp"

struct VM;

struct Pipe : public Object {
    size_t ID;
    size_t capacity;
    bool closed = false;

    std::deque<Value> buffer;

    std::deque<GreenThread*> readers;
    std::deque<GreenThread*> writers;

    std::vector<GreenThread*> selectors;

    Pipe(size_t id, size_t cap) : Object(Type::Pipe), ID(id), capacity(cap) {}

    std::string type_name() const override { return "Pipe"; }
    std::string to_string() const override { return "<pipe " + std::to_string(ID) + ">"; }
    bool is_truthy() const override { return !closed || !buffer.empty(); }
    void serialize(Serializer& serializer) override { serializer.print_pipe(this); }
    size_t object_size() const override {
        size_t size = sizeof(Pipe);
        size += buffer.size() * sizeof(Value);
        size += readers.size() * sizeof(GreenThread*);
        size += writers.size() * sizeof(GreenThread*);
        size += selectors.capacity() * sizeof(GreenThread*);
        return size;
    }

    bool can_receive();
    bool can_send();

    void notify_selectors(VM &vm);

    void send(const Value &value, VM &vm);
    Value recv(VM &vm);
    void close(VM &vm);
};

struct SelectCase {
    enum Type { Recv, Send } type;

    Pipe* pipe;
    uint8_t slot;
    Value value;
    int target_ip;
};

struct SelectFrame {
    std::vector<SelectCase> cases;
    bool has_default = false;
    int default_target_ip;

    SelectFrame(size_t case_count = 0);

    void add_recv_case(Pipe* pipe, uint16_t target_ip, uint8_t slot);
    void add_send_case(Pipe* pipe, uint16_t target_ip, Value val);
    void add_default_case(uint16_t target_ip);
    bool execute(VM &vm, int &curr_ip);
};
