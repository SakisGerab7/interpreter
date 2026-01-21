#pragma once

#include "common.hpp"
#include "value.hpp"
#include "bytecode.hpp"

// Forward declarations
struct VM;

// Function pointer type for native functions
using NativeFn = std::function<Value(VM&, const std::vector<Value> &)>;

struct Function {
    std::string name;
    Chunk chunk;
    int arity;
    int upvalue_count = 0;

    using Ptr = std::shared_ptr<Function>;

    Function(const std::string &name, int arity) : name(name), arity(arity) {}

    std::string to_string() const { return "<fn " + name + "/" + std::to_string(arity) + ">"; }
};

struct Native {
    std::string name;
    int arity;
    NativeFn func;
    Value bound_instance; // for methods

    using Ptr = std::shared_ptr<Native>;

    Native(const std::string &name, int arity, NativeFn func)
        : name(name), arity(arity), func(std::move(func)) {}

    std::string to_string() const { return "<fn " + name + "/" + std::to_string(arity) + ">"; }
};

struct Upvalue {
    Value *location;
    Value closed;

    using Ptr = std::shared_ptr<Upvalue>;

    Upvalue(Value *location) : location(location), closed() {}

    inline Value get() const {
        return (location != nullptr) ? *location : closed;
    }

    inline void set(const Value &v) {
        if (location != nullptr) {
            *location = v;
        } else {
            closed = v;
        }
    }
};

struct Closure {
    Function::Ptr func;
    std::vector<Upvalue::Ptr> upvalues;
    int upvalue_count;
    Value recv_self; // for methods

    using Ptr = std::shared_ptr<Closure>;

    Closure(Function::Ptr f) : func(std::move(f)) {
        upvalue_count = func->upvalue_count;
        upvalues.reserve(upvalue_count);
    }

    std::string to_string() const { return func->to_string(); }
};

struct Array {
    std::vector<Value> elements;

    using Ptr = std::shared_ptr<Array>;

    Array(const std::vector<Value> &elems) : elements(elems) {}

    inline size_t size() const { return elements.size(); }
    inline bool empty() const { return elements.empty(); }

    inline Value& operator[](size_t index) { return elements[index]; }
    inline const Value& operator[](size_t index) const { return elements[index]; }

    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;

    inline iterator begin() { return elements.begin(); }
    inline iterator end() { return elements.end(); }
    inline const_iterator begin() const { return elements.begin(); }
    inline const_iterator end() const { return elements.end(); }
    inline const_iterator cbegin() const { return elements.cbegin(); }
    inline const_iterator cend() const { return elements.cend(); }

    std::string to_string() const;
};

struct Object {
    std::unordered_map<std::string, Value> items;

    using Ptr = std::shared_ptr<Object>;

    Object(const std::unordered_map<std::string, Value> &map) : items(map) {}

    inline size_t size() const { return items.size(); }
    inline bool empty() const { return items.empty(); }

    inline Value& operator[](const std::string &key) { return items[key]; }
    inline const Value& operator[](const std::string &key) const { return items.at(key); }

    using iterator = std::unordered_map<std::string, Value>::iterator;
    using const_iterator = std::unordered_map<std::string, Value>::const_iterator;

    inline const_iterator find(const std::string &key) const { return items.find(key); }

    inline iterator begin() { return items.begin(); }
    inline iterator end() { return items.end(); }
    inline const_iterator begin() const { return items.begin(); }
    inline const_iterator end() const { return items.end(); }
    inline const_iterator cbegin() const { return items.cbegin(); }
    inline const_iterator cend() const { return items.cend(); }

    std::string to_string() const;
};

struct Struct {
    std::string name;
    std::unordered_map<std::string, Value> methods;

    using Ptr = std::shared_ptr<Struct>;

    Struct(const std::string &name) : name(name) {}

    inline void add_method(const std::string &name, const Value &method) {
        methods[name] = method;
    }

    std::string to_string() const { return "<struct " + name + ">"; };
};

struct StructInstance {
    std::shared_ptr<Struct> struct_ptr;
    std::unordered_map<std::string, Value> fields;

    using Ptr = std::shared_ptr<StructInstance>;

    StructInstance(std::shared_ptr<Struct> strct) : struct_ptr(std::move(strct)) {}

    inline const Value& get(const std::string &name) const {
        if (auto it = fields.find(name); it != fields.end()) return it->second;
        if (auto it2 = struct_ptr->methods.find(name); it2 != struct_ptr->methods.end()) return it2->second;
        throw std::runtime_error("Undefined property `" + std::string(name) + "`.");
    }

    inline void put(const std::string &name, const Value &value) {
        fields[name] = value;
    }

    std::string to_string() const { return "<instance of '" + std::string(struct_ptr->name) + "'>"; };
};
