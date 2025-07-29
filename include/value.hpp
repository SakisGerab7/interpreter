#pragma once

#include "common.hpp"

struct Callable;

struct Value {
    std::any Data;

    Value() = default;

    Value(int i)                       : Data(i) {}
    Value(double f)                    : Data(f) {}
    Value(bool b)                      : Data(b) {}
    Value(const std::string &s)        : Data(s) {}
    Value(std::shared_ptr<Callable> c) : Data(std::move(c)) {}
    Value(std::vector<Value> v)        : Data(std::move(v)) {}

    bool is_null()     const { return !Data.has_value(); }
    bool is_int()      const { return Data.type() == typeid(int); }
    bool is_float()    const { return Data.type() == typeid(double); }
    bool is_bool()     const { return Data.type() == typeid(bool); }
    bool is_string()   const { return Data.type() == typeid(std::string); }
    bool is_callable() const { return Data.type() == typeid(std::shared_ptr<Callable>); }
    bool is_array()    const { return Data.type() == typeid(std::vector<Value>); }

    int as_int() const {
        if (is_int()) return std::any_cast<int>(Data);
        if (is_float()) return static_cast<int>(std::any_cast<double>(Data));
        throw std::runtime_error("Value is not an int");
    }

    float as_float() const {
        if (is_float()) return std::any_cast<double>(Data);
        if (is_int()) return static_cast<double>(std::any_cast<int>(Data));
        throw std::runtime_error("Value is not a float");
    }

    bool as_bool() const {
        if (is_bool()) return std::any_cast<bool>(Data);
        throw std::runtime_error("Value is not a bool");
    }

    std::string as_string() const {
        if (is_string()) return std::any_cast<std::string>(Data);
        throw std::runtime_error("Value is not a string");
    }

    std::shared_ptr<Callable> as_callable() const {
        if (is_callable()) return std::any_cast<std::shared_ptr<Callable>>(Data);
        throw std::runtime_error("Value is not a callable");
    }

    std::vector<Value> as_array() const {
        if (is_array()) return std::any_cast<std::vector<Value>>(Data);
        throw std::runtime_error("Value is not an array");
    }

    std::string to_string() const {
        if (is_int())      return std::to_string(as_int());
        if (is_float())    return std::to_string(as_float());
        if (is_bool())     return as_bool() ? "true" : "false";
        if (is_string())   return as_string();
        if (is_callable()) return "<function>";
        return "null";
    }

    bool is_truthy() const {
        if (is_int())      return as_int() != 0;
        if (is_float())    return as_float() != 0;
        if (is_bool())     return as_bool();
        if (is_string())   return !as_string().empty();
        if (is_callable()) return true;
        if (is_array())    return !as_array().empty();
        return false;
    }
};

Value operator+(const Value &lhs, const Value &rhs);
Value operator-(const Value &lhs, const Value &rhs);
Value operator*(const Value &lhs, const Value &rhs);
Value operator/(const Value &lhs, const Value &rhs);
Value operator%(const Value &lhs, const Value &rhs);

Value operator-(const Value &v);

bool operator==(const Value &lhs, const Value &rhs);
bool operator!=(const Value &lhs, const Value &rhs);
bool operator<(const Value &lhs, const Value &rhs);
bool operator<=(const Value &lhs, const Value &rhs);
bool operator>(const Value &lhs, const Value &rhs);
bool operator>=(const Value &lhs, const Value &rhs);

bool logical_and(const Value &lhs, const Value &rhs);
bool logical_or(const Value &lhs, const Value &rhs);
bool logical_not(const Value &v);
