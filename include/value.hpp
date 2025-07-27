#pragma once


#include "common.hpp"

struct Value {
    std::any Data;

    Value() = default;

    Value(int i)   : Data(i) {}
    Value(float f) : Data(f) {}
    Value(bool b)  : Data(b) {}
    Value(const std::string &s) : Data(s) {}

    bool is_null()   const { return !Data.has_value(); }
    bool is_int()    const { return Data.type() == typeid(int); }
    bool is_float()  const { return Data.type() == typeid(float); }
    bool is_bool()   const { return Data.type() == typeid(bool); }
    bool is_string() const { return Data.type() == typeid(std::string); }

    int as_int() const {
        if (is_int()) return std::any_cast<int>(Data);
        if (is_float()) return static_cast<int>(std::any_cast<float>(Data));
        throw std::runtime_error("Not an integer");
    }

    float as_float() const {
        if (is_float()) return std::any_cast<float>(Data);
        if (is_int()) return static_cast<float>(std::any_cast<int>(Data));
        throw std::runtime_error("Not a float");
    }

    bool as_bool() const {
        if (is_bool()) return std::any_cast<bool>(Data);
        throw std::runtime_error("Not a bool");
    }

    std::string as_string() const {
        if (is_string()) return std::any_cast<std::string>(Data);
        throw std::runtime_error("Not a string");
    }

    std::string to_string() const {
        if (is_int())    return std::to_string(as_int());
        if (is_float())  return std::to_string(as_float());
        if (is_bool())   return as_bool() ? "true" : "false";
        if (is_string()) return as_string();
        return "null";
    }

    bool operator==(const Value& other) const {
        if (Data.type() != other.Data.type()) return false;
        if (is_int())    return as_int()    == other.as_int();
        if (is_float())  return as_float()  == other.as_float();
        if (is_bool())   return as_bool()   == other.as_bool();
        if (is_string()) return as_string() == other.as_string();
        return true;
    }

    bool is_truthy() const {
        if (is_int())    return as_int()   != 0;
        if (is_float())  return as_float() != 0;
        if (is_bool())   return as_bool();
        if (is_string()) return !as_string().empty();
        return true;
    }
};