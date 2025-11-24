#pragma once

#include "common.hpp"
#include "token.hpp"

struct Function;
struct Native;
struct Closure;
struct Array;
struct Object;
struct Struct;
struct StructInstance;
struct Upvalue;

struct ThreadHandle {
    size_t ID;

    ThreadHandle(size_t id) : ID(id) {}
};

struct Value {
    std::any data;

    using FunctionPtr = std::shared_ptr<Function>;
    using NativePtr   = std::shared_ptr<Native>;
    using ClosurePtr  = std::shared_ptr<Closure>;
    using ArrayPtr    = std::shared_ptr<Array>;
    using ObjectPtr   = std::shared_ptr<Object>;
    using StructPtr   = std::shared_ptr<Struct>;
    using StructInstancePtr = std::shared_ptr<StructInstance>;
    using UpvaluePtr  = std::shared_ptr<Upvalue>;

    Value()         : data()  {}
    Value(int i)    : data(i) {}
    Value(double f) : data(f) {}
    Value(bool b)   : data(b) {}

    Value(const std::string &s) : data(s) {}
    Value(std::string &&s)      : data(std::move(s)) {}
    Value(const char *s)        : data(std::string(s)) {}
    
    Value(FunctionPtr f)          : data(std::move(f)) {}
    Value(NativePtr n)            : data(std::move(n)) {}
    Value(ClosurePtr c)           : data(std::move(c)) {}
    Value(ArrayPtr arr)           : data(std::move(arr)) {}
    Value(ObjectPtr obj)          : data(std::move(obj)) {}
    Value(StructPtr strct)        : data(std::move(strct)) {}
    Value(StructInstancePtr inst) : data(std::move(inst)) {}
    Value(UpvaluePtr upval)       : data(std::move(upval)) {}

    Value(ThreadHandle th) : data(th) {}

    template <typename T>
    inline bool is() const { return data.type() == typeid(T); }

    inline bool is_null()     const { return !data.has_value(); }
    inline bool is_int()      const { return is<int>(); }
    inline bool is_float()    const { return is<double>(); }
    inline bool is_bool()     const { return is<bool>(); }
    inline bool is_string()   const { return is<std::string>(); }
    inline bool is_function() const { return is<FunctionPtr>(); }
    inline bool is_native()   const { return is<NativePtr>(); }
    inline bool is_closure()  const { return is<ClosurePtr>(); }
    inline bool is_array()    const { return is<ArrayPtr>(); }
    inline bool is_object()   const { return is<ObjectPtr>(); }
    inline bool is_struct()   const { return is<StructPtr>(); }
    inline bool is_struct_instance() const { return is<StructInstancePtr>(); }
    inline bool is_thread_handle()   const { return is<ThreadHandle>(); }
    inline bool is_upvalue()         const { return is<UpvaluePtr>(); }

    inline int as_int() const {
        if (is_int()) return std::any_cast<int>(data);
        if (is_float()) return static_cast<int>(std::any_cast<double>(data));
        throw std::runtime_error("Value is not an int");
    }
    
    inline double as_float() const {
        if (is_float()) return std::any_cast<double>(data);
        if (is_int()) return static_cast<double>(std::any_cast<int>(data));
        throw std::runtime_error("Value is not a float");
    }

    template <typename T>
    inline T as(const std::string &type_name) const {
        if (is<T>()) return std::any_cast<T>(data);
        throw std::runtime_error("Value is not a " + type_name);
    }

    inline bool as_bool() const { return as<bool>("bool"); }
    inline const std::string& as_string()   const { return as<const std::string&>("string"); }
    inline const FunctionPtr& as_function() const { return as<const FunctionPtr&>("function"); }
    inline const NativePtr& as_native()     const { return as<const NativePtr&>("native function"); }
    inline const ClosurePtr& as_closure()   const { return as<const ClosurePtr&>("closure"); }
    inline const ArrayPtr& as_array()       const { return as<const ArrayPtr&>("array"); }
    inline const ObjectPtr& as_object()     const { return as<const ObjectPtr&>("object"); }
    inline const StructPtr& as_struct()     const { return as<const StructPtr&>("struct"); }
    inline const StructInstancePtr& as_struct_instance() const { return as<const StructInstancePtr&>("struct instance"); }
    inline ThreadHandle as_thread_handle() const { return as<ThreadHandle>("thread handle"); }
    inline const UpvaluePtr& as_upvalue()   const { return as<const UpvaluePtr&>("upvalue"); }

    Value get_index(const Value &idx) const;
    void set_index(const Value &idx, const Value &val);
    
    std::string type_name() const;
    std::string to_string() const;
    
    bool is_truthy() const;
    operator bool() const { return is_truthy(); }
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;

    inline void write_u8(uint8_t v) {
        code.push_back(v);
    }

    inline void write_u16(uint16_t v) {
        code.push_back((v >> 8) & 0xFF);
        code.push_back(v & 0xFF);
    }

    uint16_t add_constant(const Value &v);
};

struct Function {
    std::string name;
    Chunk chunk;
    int arity;
    int upvalue_count = 0;

    using Ptr = std::shared_ptr<Function>;

    Function(const std::string &name, int arity) : name(name), arity(arity) {}

    std::string to_string() const { return "<fn " + name + "/" + std::to_string(arity) + ">"; }
};

struct VM;

using NativeFn = std::function<Value(VM&, const std::vector<Value> &)>;

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

Value operator+(const Value &lhs, const Value &rhs);
Value operator-(const Value &lhs, const Value &rhs);
Value operator*(const Value &lhs, const Value &rhs);
Value operator/(const Value &lhs, const Value &rhs);
Value operator%(const Value &lhs, const Value &rhs);

Value operator-(const Value &v);
Value operator~(const Value &v);

bool operator==(const Value &lhs, const Value &rhs);
bool operator!=(const Value &lhs, const Value &rhs);
bool operator<(const Value &lhs, const Value &rhs);
bool operator<=(const Value &lhs, const Value &rhs);
bool operator>(const Value &lhs, const Value &rhs);
bool operator>=(const Value &lhs, const Value &rhs);

bool operator&&(const Value &lhs, const Value &rhs);
bool operator||(const Value &lhs, const Value &rhs);
bool operator!(const Value &v);

Value operator|(const Value &lhs, const Value &rhs);
Value operator^(const Value &lhs, const Value &rhs);
Value operator&(const Value &lhs, const Value &rhs);
Value operator<<(const Value &lhs, const Value &rhs);
Value operator>>(const Value &lhs, const Value &rhs);