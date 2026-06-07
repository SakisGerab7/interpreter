#pragma once

#include "common.hpp"
#include "token.hpp"

struct Object;
struct Function;
struct Native;
struct Closure;
struct MethodClosure;
struct Upvalue;
struct Array;
struct Record;
struct ByteArray;
struct Struct;
struct StructInstance;
struct GreenThread;
struct IOHandle;
struct Pipe;

struct Value {
    std::variant<
        std::monostate, // for null
        int,
        double,
        bool,
        std::string,
        Object*
    > data;

    Value() : data(std::monostate{}) {}
    Value(int i) : data(i) {}
    Value(double f) : data(f) {}
    Value(bool b) : data(b) {}
    Value(const std::string &s) : data(s) {}
    Value(std::string &&s) : data(std::move(s)) {}
    Value(const char *s) : data(std::string(s)) {}
    Value(Object* obj) : data(obj) {}

    inline bool is_null()   const { return std::holds_alternative<std::monostate>(data); }
    inline bool is_int()    const { return std::holds_alternative<int>(data);            }
    inline bool is_float()  const { return std::holds_alternative<double>(data);         }
    inline bool is_bool()   const { return std::holds_alternative<bool>(data);           }
    inline bool is_string() const { return std::holds_alternative<std::string>(data);    }
    inline bool is_object() const { return std::holds_alternative<Object*>(data);        }

    bool is_function() const;
    bool is_native() const;
    bool is_closure() const;
    bool is_method_closure() const;
    bool is_upvalue() const;
    bool is_array() const;
    bool is_record() const;
    bool is_byte_array() const;
    bool is_struct() const;
    bool is_struct_instance() const;
    bool is_thread() const;
    bool is_io_handle() const;
    bool is_pipe() const;

    int as_int() const;
    double as_float() const;
    bool as_bool() const;
    const std::string& as_string() const;
    Object* as_object() const;

    Function* as_function() const;
    Native* as_native() const;
    Closure* as_closure() const;
    MethodClosure* as_method_closure() const;
    Upvalue* as_upvalue() const;
    Array* as_array() const;
    Record* as_record() const;
    ByteArray* as_byte_array() const;
    Struct* as_struct() const;
    StructInstance* as_struct_instance() const;
    GreenThread* as_thread() const;
    IOHandle* as_io_handle() const;
    Pipe* as_pipe() const;

    std::string type_name() const;
    std::string to_string() const;

    Value get_index(const Value &idx);
    void set_index(const Value &idx, const Value &val);

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

Value multiply_add(const Value &a, const Value &b, const Value &c);
