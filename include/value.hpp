#pragma once

#include "common.hpp"

struct Callable;

struct Value {
    using CallablePtr = std::shared_ptr<Callable>;

    using Array = std::vector<Value>;
    using Object = std::unordered_map<std::string, std::shared_ptr<Value>>;

    using ValueData = std::variant<
        std::monostate,
        int,
        double,
        bool,
        std::string,
        CallablePtr,
        Array,
        Object
    >;

    ValueData data;

    Value()                     : data(std::monostate{}) {}
    Value(int i)                : data(i) {}
    Value(double f)             : data(f) {}
    Value(bool b)               : data(b) {}
    Value(const std::string &s) : data(s) {}
    Value(std::string &&s)      : data(std::move(s)) {}
    Value(const char* s)        : data(std::string(s)) {}
    Value(CallablePtr c)        : data(std::move(c)) {}
    Value(const Array &arr)     : data(arr) {}
    Value(Array &&arr)          : data(std::move(arr)) {}
    Value(const Object &obj)    : data(obj) {}
    Value(Object &&obj)         : data(std::move(obj)) {}

    inline bool is_null()     const { return std::holds_alternative<std::monostate>(data); }
    inline bool is_int()      const { return std::holds_alternative<int>(data); }
    inline bool is_float()    const { return std::holds_alternative<double>(data); }
    inline bool is_bool()     const { return std::holds_alternative<bool>(data); }
    inline bool is_string()   const { return std::holds_alternative<std::string>(data); }
    inline bool is_callable() const { return std::holds_alternative<CallablePtr>(data); }
    inline bool is_array()    const { return std::holds_alternative<Array>(data); }
    inline bool is_object()   const { return std::holds_alternative<Object>(data); }
    
    inline int as_int() const {
        if (is_int()) return std::get<int>(data);
        if (is_float()) return static_cast<int>(std::get<double>(data));
        throw std::runtime_error("Value is not an int");
    }
    
    inline double as_float() const {
        if (is_float()) return std::get<double>(data);
        if (is_int()) return static_cast<double>(std::get<int>(data));
        throw std::runtime_error("Value is not a float");
    }
    
    inline bool as_bool() const {
        if (is_bool()) return std::get<bool>(data);
        throw std::runtime_error("Value is not a bool");
    }
    
    inline const std::string& as_string() const {
        if (is_string()) return std::get<std::string>(data);
        throw std::runtime_error("Value is not a string");
    }
    
    inline const CallablePtr& as_callable() const {
        if (is_callable()) return std::get<CallablePtr>(data);
        throw std::runtime_error("Value is not a callable");
    }
    
    inline const Array& as_array() const {
        if (is_array()) return std::get<Array>(data);
        throw std::runtime_error("Value is not an array");
    }

    inline const Object& as_object() const {
        if (is_object()) return std::get<Object>(data);
        throw std::runtime_error("Value is not an object");
    }

    void set_index(int index, const Value &val) {
        if (!is_array()) throw std::runtime_error("Value is not an array");

        if (index < 0) throw std::runtime_error("Negative index assignment not supported");

        Array &arr = std::get<Array>(data);

        if (static_cast<size_t>(index) >= arr.size()) {
            arr.resize(index + 1);
        }

        arr[index] = val;
    }

    void set_index(const std::string &key, const Value &val) {
        if (!is_object()) throw std::runtime_error("Value is not an object");
        std::get<Object>(data)[key] = std::make_shared<Value>(val);
    }

    std::string to_string() const {
        if (is_int())      return std::to_string(as_int());
        if (is_float())    return std::to_string(as_float());
        if (is_bool())     return as_bool() ? "true" : "false";
        if (is_string())   return as_string();
        if (is_callable()) return "<function>";

        if (is_array()) {
            const auto &arr = as_array();
            std::string result = "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                result += arr[i].to_string();
                if (i < arr.size() - 1) result += ", ";
            }

            result += "]";
            return result;
        }
    
        if (is_object()) {
            auto &obj = as_object();
            std::string result = "{";
            size_t count = 0;
            for (const auto &[key, value] : obj) {
                result += "\"" + key + "\": " + value->to_string();
                if (count < obj.size() - 1) result += ", ";
                ++count;
            }
            
            result += "}";
            return result;
        }

        return "null";
    }

    bool is_truthy() const {
        if (is_null())     return false;
        if (is_int())      return as_int() != 0;
        if (is_float())    return as_float() != 0;
        if (is_bool())     return as_bool();
        if (is_string())   return !as_string().empty();
        if (is_callable()) return true;
        if (is_array())    return !as_array().empty();
        return false;
    }

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