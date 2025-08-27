#include "value.hpp"
#include "callable.hpp"

std::string Array::to_string() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < Elements.size(); ++i) {
        ss << Elements[i].to_string();
        if (i < Elements.size() - 1) ss << ", ";
    }

    ss << "]";
    return ss.str();
}

std::string Object::to_string() const {
    std::stringstream ss;
    ss << "{";
    size_t count = 0;
    for (const auto &[key, value] : Items) {
        ss << "\"" << key << "\": " << value.to_string();
        if (count < Items.size() - 1) ss << ", ";
        count++;
    }
    
    ss << "}";
    return ss.str();
}

void Value::set_index(int index, const Value &val) {
    if (!is_array()) throw std::runtime_error("Value is not an array");
    if (index < 0) throw std::runtime_error("Negative index assignment not supported");
    Array &arr = *std::get<ArrayPtr>(data);
    arr[static_cast<size_t>(index)] = val;
}

void Value::set_index(const std::string &key, const Value &val) {
    if (is_object()) {
        Object &obj = *std::get<ObjectPtr>(data);
        obj[key] = val;
    } else if (is_struct_instance()) {
        std::get<StructInstancePtr>(data)->put(key, val);
    } else {
        throw std::runtime_error("Value is neither an object nor a struct instance");
    }
}

std::string Value::to_string() const {
    if (is_int())      return std::to_string(as_int());
    if (is_float())    return std::to_string(as_float());
    if (is_bool())     return as_bool() ? "true" : "false";
    if (is_string())   return as_string();
    if (is_callable()) return as_callable()->to_string();
    if (is_array())    return as_array()->to_string();
    if (is_object())   return as_object()->to_string();
    if (is_struct())   return as_struct()->to_string();
    if (is_struct_instance()) return as_struct_instance()->to_string();

    return "null";
}

bool Value::is_truthy() const {
    if (is_null())     return false;
    if (is_int())      return as_int() != 0;
    if (is_float())    return as_float() != 0;
    if (is_bool())     return as_bool();
    if (is_string())   return !as_string().empty();
    if (is_callable()) return true;
    if (is_array())    return !as_array()->empty();
    return false;
}

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

Value operator~(const Value &v) {
    if (v.is_int()) return ~v.as_int();
    throw std::runtime_error("Unsupported type for '~'");
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
        if (a1->size() != a2->size()) return false;
        for (size_t i = 0; i < a1->size(); ++i) {
            if (!((*a1)[i] == (*a2)[i])) return false;
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

bool operator&&(const Value &lhs, const Value &rhs) {
    return lhs.is_truthy() && rhs.is_truthy();
}

bool operator||(const Value &lhs, const Value &rhs) {
    return lhs.is_truthy() || rhs.is_truthy();
}

bool operator!(const Value &v) {
    return !v.is_truthy();
}

Value operator|(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() | rhs.as_int();
    throw std::runtime_error("Unsupported types for '|'");
}

Value operator^(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() ^ rhs.as_int();
    throw std::runtime_error("Unsupported types for '^'");
}

Value operator&(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() & rhs.as_int();
    throw std::runtime_error("Unsupported types for '&'");
}

Value operator<<(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() << rhs.as_int();
    throw std::runtime_error("Unsupported types for '<<'");
}

Value operator>>(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() >> rhs.as_int();
    throw std::runtime_error("Unsupported types for '>>'");
}

