#include "value.hpp"
#include "bytecode.hpp"
#include "runtime.hpp"
#include "threading.hpp"

std::string Array::to_string() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        ss << elements[i].to_string();
        if (i < elements.size() - 1) ss << ", ";
    }

    ss << "]";
    return ss.str();
}

std::string Object::to_string() const {
    std::stringstream ss;
    ss << "{";
    size_t count = 0;
    for (const auto &[key, value] : items) {
        ss << "\"" << key << "\": " << value.to_string();
        if (count < items.size() - 1) ss << ", ";
        count++;
    }
    
    ss << "}";
    return ss.str();
}

Value Value::get_index(const Value &idx) const {
    if (is_array() && idx.is_int()) {
        int i = idx.as_int();
        if (i < 0) 
            throw std::runtime_error("Negative index access not supported");

        const Array &arr = *std::any_cast<Array::Ptr>(data);

        if (static_cast<size_t>(i) >= arr.size())
            throw std::runtime_error("Array index out of bounds");

        return arr[static_cast<size_t>(i)];
    } else if (idx.is_string()) {
        std::string k = idx.as_string();
        if (is_object()) {
            const Object &obj = *std::any_cast<Object::Ptr>(data);
            auto it = obj.find(k);
            if (it == obj.end())
                throw std::runtime_error("Key '" + k + "' not found in object");

            return it->second;
        } else if (is_struct_instance()) {
            auto &instance = *std::any_cast<StructInstance::Ptr>(data);
            return instance.get(k);
        } else {
            throw std::runtime_error("Cannot access with string key: container type=" 
                                     + type_name() + ", key=" + k);
        }
    } 
    else {
        throw std::runtime_error("Invalid index access: container type=" 
                                 + type_name() + ", index type=" + idx.type_name());
    }
}

void Value::set_index(const Value &idx, const Value &val) {
    if (is_array() && idx.is_int()) {
        int i = idx.as_int();
        if (i < 0) 
            throw std::runtime_error("Negative index assignment not supported");

        Array &arr = *std::any_cast<Array::Ptr>(data);
        arr[static_cast<size_t>(i)] = val;
    } else if (idx.is_string()) {
        std::string k = idx.as_string();
        if (is_object()) {
            Object &obj = *std::any_cast<Object::Ptr>(data);
            obj[k] = val;
        } else if (is_struct_instance()) {
            auto &instance = *std::any_cast<StructInstance::Ptr>(data);
            instance.put(k, val);
        } else {
            throw std::runtime_error("Cannot assign with string key: container type=" 
                                     + type_name() + ", key=" + k);
        }
    } else {
        throw std::runtime_error("Invalid index assignment: container type=" 
                                 + type_name() + ", index type=" + idx.type_name());
    }
}

std::string Value::type_name() const {
    if (is_null()) return "null";
    if (is_bool()) return "bool";
    if (is_int()) return "int";
    if (is_float()) return "float";
    if (is_string()) return "string";
    if (is_function()) return "function";
    if (is_native()) return "native function";
    if (is_closure()) return "closure";
    if (is_array()) return "array";
    if (is_object()) return "object";
    if (is_struct()) return "struct";
    if (is_struct_instance()) return "struct instance";
    if (is_thread_handle()) return "thread handle";
    if (is_pipe_handle()) return "pipe handle";
    if (is_upvalue()) return "upvalue";
    return "unknown";
}

std::string Value::to_string() const {
    if (is_int())      return std::to_string(as_int());
    if (is_float())    return std::to_string(as_float());
    if (is_bool())     return as_bool() ? "true" : "false";
    if (is_string())   return as_string();
    if (is_function()) return as_function()->to_string();
    if (is_native())   return as_native()->to_string();
    if (is_closure())  return as_closure()->to_string();
    if (is_array())    return as_array()->to_string();
    if (is_object())   return as_object()->to_string();
    if (is_struct())   return as_struct()->to_string();
    if (is_struct_instance()) return as_struct_instance()->to_string();
    if (is_thread_handle()) return "thread " + std::to_string(as_thread_handle().ID);
    if (is_pipe_handle()) return "pipe " + std::to_string(as_pipe_handle().ID);
    if (is_upvalue()) return as_upvalue()->get().to_string();
    return "null";
}

bool Value::is_truthy() const {
    if (is_null())     return false;
    if (is_int())      return as_int() != 0;
    if (is_float())    return as_float() != 0;
    if (is_bool())     return as_bool();
    if (is_string())   return !as_string().empty();
    if (is_function()) return true;
    if (is_native())   return true;
    if (is_closure())  return true;
    if (is_array())    return !as_array()->empty();
    if (is_object())   return !as_object()->empty();
    if (is_struct())   return true;
    if (is_struct_instance()) return true;
    if (is_thread_handle())   return true;
    if (is_pipe_handle()) {
        auto pipe = as_pipe_handle().pipe_ptr;
        // std::cout << "Checking pipe truthiness: closed=" << pipe->closed << ", buffer size=" << pipe->buffer.size() << "\n";
        return pipe && (!pipe->buffer.empty() || !pipe->closed);
    }
    if (is_upvalue())         return as_upvalue()->get().is_truthy();
    return false;
}

Value operator+(const Value &lhs, const Value &rhs) {
    if (lhs.is_int() && rhs.is_int())
        return lhs.as_int() + rhs.as_int();
    if ((lhs.is_int() || lhs.is_float()) && (rhs.is_int() || rhs.is_float()))
        return lhs.as_float() + rhs.as_float();
    if (lhs.is_string() || rhs.is_string())
        return lhs.to_string() + rhs.to_string();
    if (lhs.is_array() && rhs.is_array()) {
        const auto &arr1 = lhs.as_array();
        const auto &arr2 = rhs.as_array();
        std::vector<Value> combined;
        combined.reserve(arr1->size() + arr2->size());
        combined.insert(combined.end(), arr1->begin(), arr1->end());
        combined.insert(combined.end(), arr2->begin(), arr2->end());
        return std::make_shared<Array>(combined);
    }

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
    if ((lhs.is_array() && rhs.is_int()) || (lhs.is_int() && rhs.is_array())) { 
        const Value &arr_val = lhs.is_array() ? lhs : rhs;
        const Value &int_val = lhs.is_int() ? lhs : rhs;
        const auto &arr = arr_val.as_array();
        int times = int_val.as_int();
        if (times < 0) throw std::runtime_error("Cannot multiply array by negative integer");

        std::vector<Value> result;
        result.reserve(arr->size() * times);
        for (int i = 0; i < times; ++i) {
            result.insert(result.end(), arr->begin(), arr->end());
        }
        return std::make_shared<Array>(result);
    }
    if ((lhs.is_string() && rhs.is_int()) || (lhs.is_int() && rhs.is_string())) {
        const Value &str_val = lhs.is_string() ? lhs : rhs;
        const Value &int_val = lhs.is_int() ? lhs : rhs;
        const std::string &s = str_val.as_string();
        int times = int_val.as_int();
        if (times < 0) throw std::runtime_error("Cannot multiply string by negative integer");

        std::string result;
        for (int i = 0; i < times; ++i) {
            result += s;
        }
        return result;
    }

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

    if (lhs.is_function() && rhs.is_function())
        return lhs.as_function() == rhs.as_function();
    
    if (lhs.is_native() && rhs.is_native())
        return lhs.as_native() == rhs.as_native();

    if (lhs.is_closure() && rhs.is_closure())
        return lhs.as_closure() == rhs.as_closure();

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