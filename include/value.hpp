#pragma once

#include "common.hpp"
#include "token.hpp"

// Forward declarations for runtime types
struct Function;
struct Native;
struct Closure;
struct Array;
struct Object;
struct Struct;
struct StructInstance;
struct Upvalue;
struct Pipe;

// Simple handle types for concurrency
struct ThreadHandle {
    size_t ID;
    ThreadHandle(size_t id) : ID(id) {}
};

struct PipeHandle {
    size_t ID;
    std::shared_ptr<Pipe> pipe_ptr;
    PipeHandle(size_t id, std::shared_ptr<Pipe> ptr = nullptr) : ID(id), pipe_ptr(std::move(ptr)) {}
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
    Value(PipeHandle ph)   : data(ph) {}

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
    inline bool is_pipe_handle()     const { return is<PipeHandle>(); }
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
    inline PipeHandle as_pipe_handle()     const { return as<PipeHandle>("pipe handle"); }
    inline const UpvaluePtr& as_upvalue()   const { return as<const UpvaluePtr&>("upvalue"); }

    Value get_index(const Value &idx) const;
    void set_index(const Value &idx, const Value &val);
    
    std::string type_name() const;
    std::string to_string() const;
    
    bool is_truthy() const;
    operator bool() const { return is_truthy(); }
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