#include "value.hpp"

Value operator+(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() + rhs.as_int();
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float()))
        return lhs.as_float() + rhs.as_float();
    if (lhs.is_string() || rhs.is_string())
        return lhs.to_string() + rhs.to_string();

    throw std::runtime_error("Unsupported types for '+'");
}

Value operator-(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() - rhs.as_int();
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float()))
        return lhs.as_float() - rhs.as_float();
        
    throw std::runtime_error("Unsupported types for '-'");
}

Value operator*(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() * rhs.as_int();
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float()))
        return lhs.as_float() * rhs.as_float();

    throw std::runtime_error("Unsupported types for '*'");
}

Value operator/(const Value &lhs, const Value &rhs) {
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float())) {
        double denom = rhs.as_float();
        if (denom == 0) throw std::runtime_error("Division by zero");
        return lhs.as_float() / denom;
    }

    throw std::runtime_error("Unsupported types for '/'");
}

Value operator%(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int()) {
        int divisor = rhs.as_int();
        if (divisor == 0) throw std::runtime_error("Modulo by zero");
        return lhs.as_int() % divisor;
    }

    throw std::runtime_error("Unsupported types for '%'");
}

Value operator-(const Value &v) {
    if (v.is_int())   return -v.as_int();
    if (v.is_float()) return -v.as_float();
    throw std::runtime_error("Unary '-' operator requires a numeric value.");
}

bool operator==(const Value &lhs, const Value &rhs) {
    if (lhs.is_null() && rhs.is_null()) return true;
    if (lhs.is_null() || rhs.is_null()) return false;

    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float())) {
        return lhs.as_float() == rhs.as_float();
    }

    if (lhs.is_bool() && rhs.is_bool())
        return lhs.as_bool() == rhs.as_bool();

    if (lhs.is_string() && rhs.is_string())
        return lhs.as_string() == rhs.as_string();

    if (lhs.is_callable() && rhs.is_callable())
        return lhs.as_callable() == rhs.as_callable();

    if (lhs.is_array() && rhs.is_array()) {
        const auto &a1 = lhs.as_array();
        const auto &a2 = rhs.as_array();
        if (a1.size() != a2.size()) return false;
        for (size_t i = 0; i < a1.size(); ++i) {
            if (!(a1[i] == a2[i])) return false;
        }

        return true;
    }

    return false;
}

bool operator!=(const Value &lhs, const Value &rhs) {
    return !(lhs == rhs);
}

bool operator<(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() < rhs.as_int();
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float()))
        return lhs.as_float() < rhs.as_float();
    if (lhs.is_string() && rhs.is_string())
        return lhs.as_string() < rhs.as_string();

    throw std::runtime_error("Unsupported types for '<'");
}

bool operator<=(const Value &lhs, const Value &rhs) {
    return lhs < rhs || lhs == rhs;
}

bool operator>(const Value &lhs, const Value &rhs) {
    return !(lhs <= rhs);
}

bool operator>=(const Value &lhs, const Value &rhs) {
    return !(lhs < rhs);
}

bool logical_and(const Value &lhs, const Value &rhs) {
    return lhs.is_truthy() && rhs.is_truthy();
}

bool logical_or(const Value &lhs, const Value &rhs) {
    return lhs.is_truthy() || rhs.is_truthy();
}

bool logical_not(const Value &v) {
    return !v.is_truthy();
}