#include "value.hpp"
#include "bytecode.hpp"
#include "runtime.hpp"
#include "pipe.hpp"
#include "io_handler.hpp"
#include "vm.hpp"

inline std::string Array::to_string() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        ss << elements[i].to_string();
        if (i < elements.size() - 1) ss << ", ";
    }

    ss << "]";
    return ss.str();
}

inline std::string Record::to_string() const {
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

inline std::string ByteArray::to_string() const {
    std::stringstream ss;
    ss << "0x";
    for (uint8_t byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

bool Value::is_function()        const { return is_object() && as_object()->is_function();        }
bool Value::is_native()          const { return is_object() && as_object()->is_native();          }
bool Value::is_closure()         const { return is_object() && as_object()->is_closure();         }
bool Value::is_method_closure()  const { return is_object() && as_object()->is_method_closure();  }
bool Value::is_upvalue()         const { return is_object() && as_object()->is_upvalue();         }
bool Value::is_array()           const { return is_object() && as_object()->is_array();           }
bool Value::is_record()          const { return is_object() && as_object()->is_record();          }
bool Value::is_byte_array()      const { return is_object() && as_object()->is_byte_array();      }
bool Value::is_struct()          const { return is_object() && as_object()->is_struct();          }
bool Value::is_struct_instance() const { return is_object() && as_object()->is_struct_instance(); }
bool Value::is_thread()          const { return is_object() && as_object()->is_thread();          }
bool Value::is_io_handle()       const { return is_object() && as_object()->is_io_handle();       }
bool Value::is_pipe()            const { return is_object() && as_object()->is_pipe();            }

int Value::as_int() const {
    if (is_int()) return std::get<int>(data);
    if (is_float()) return static_cast<int>(std::get<double>(data));
    throw std::runtime_error("Value is not an int");
}

double Value::as_float() const {
    if (is_float()) return std::get<double>(data);
    if (is_int()) return static_cast<double>(std::get<int>(data));
    throw std::runtime_error("Value is not a float");
}

bool Value::as_bool() const {
    if (is_bool()) return std::get<bool>(data);
    throw std::runtime_error("Value is not a bool");
}

const std::string& Value::as_string() const {
    if (is_string()) return std::get<std::string>(data);
    throw std::runtime_error("Value is not a string");
}

Object* Value::as_object() const {
    if (is_object()) return std::get<Object*>(data);
    throw std::runtime_error("Value is not an object");
}

Function* Value::as_function() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_function()) return obj->as_function();
    }
    throw std::runtime_error("Value is not a function");
}

Native* Value::as_native() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_native()) return obj->as_native();
    }
    throw std::runtime_error("Value is not a native function");
}

Closure* Value::as_closure() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_closure()) return obj->as_closure();
    }
    throw std::runtime_error("Value is not a closure");
}

MethodClosure* Value::as_method_closure() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_method_closure()) return obj->as_method_closure();
    }
    throw std::runtime_error("Value is not a method closure");
}

Upvalue* Value::as_upvalue() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_upvalue()) return obj->as_upvalue();
    }
    throw std::runtime_error("Value is not an upvalue");
}

Array* Value::as_array() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_array()) return obj->as_array();
    }
    throw std::runtime_error("Value is not an array");
}

Record* Value::as_record() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_record()) return obj->as_record();
    }
    throw std::runtime_error("Value is not a record");
}

ByteArray* Value::as_byte_array() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_byte_array()) return obj->as_byte_array();
    }
    throw std::runtime_error("Value is not a byte array");
}

Struct* Value::as_struct() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_struct()) return obj->as_struct();
    }
    throw std::runtime_error("Value is not a struct");
}

StructInstance* Value::as_struct_instance() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_struct_instance()) return obj->as_struct_instance();
    }
    throw std::runtime_error("Value is not a struct instance");
}

GreenThread* Value::as_thread() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_thread()) return obj->as_thread();
    }
    throw std::runtime_error("Value is not a thread");
}

IOHandle* Value::as_io_handle() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_io_handle()) return obj->as_io_handle();
    }
    throw std::runtime_error("Value is not an I/O handle");
}

Pipe* Value::as_pipe() const {
    if (is_object()) {
        Object* obj = std::get<Object*>(data);
        if (obj->is_pipe()) return obj->as_pipe();
    }
    throw std::runtime_error("Value is not a pipe");
}

Value Value::get_index(const Value &idx) {
    if (idx.is_int()) {
        int i = idx.as_int();
        if (i < 0)
            throw std::runtime_error("Negative index access not supported");

        if (is_array()) {
            Array* arr = as_array();

            if (static_cast<size_t>(i) >= arr->size())
                throw std::runtime_error("Array index out of bounds");

            return (*arr)[static_cast<size_t>(i)];
        }

        if (is_string()) {
            const std::string &str = std::get<std::string>(data);

            if (static_cast<size_t>(i) >= str.size())
                throw std::runtime_error("String index out of bounds");

            return std::string(1, str[i]);
        }
    }

    if (idx.is_string()) {
        std::string k = idx.as_string();

        if (is_record()) {
            Record* record = as_record();
            auto it = record->find(k);
            if (it == record->end())
                throw std::runtime_error("Key '" + k + "' not found in record");

            return it->second;
        }

        if (is_struct_instance()) {
            StructInstance* instance = as_struct_instance();
            return instance->get(k);
        }

        throw std::runtime_error("Cannot access with string key: container type="
                                 + type_name() + ", key=" + k);
    }

    throw std::runtime_error("Invalid index access: container type="
                             + type_name() + ", index type=" + idx.type_name());
}

void Value::set_index(const Value &idx, const Value &val) {
    if (idx.is_int()) {
        int i = idx.as_int();
        if (i < 0)
            throw std::runtime_error("Negative index assignment not supported");

        if (is_array()) {
            Array* arr = as_array();
            (*arr)[static_cast<size_t>(i)] = val;
            return;
        }

        if (is_string()) {
            throw std::runtime_error("Strings are immutable, cannot assign to index");
        }
    }

    if (idx.is_string()) {
        std::string k = idx.as_string();
        if (is_record()) {
            Record* record = as_record();
            (*record)[k] = val;
            return;
        }

        if (is_struct_instance()) {
            StructInstance* instance = as_struct_instance();
            instance->put(k, val);
            return;
        }

        throw std::runtime_error("Cannot assign with string key: container type=" + type_name() + ", key=" + k);

        return;
    }

    throw std::runtime_error("Invalid index assignment: container type="
                             + type_name() + ", index type=" + idx.type_name());
}

std::string Value::type_name() const {
    switch (data.index()) {
        case 0: return "null";
        case 1: return "int";
        case 2: return "float";
        case 3: return "bool";
        case 4: return "string";
        case 5: return as_object()->type_name();
        default: return "unknown";
    }
}

std::string Value::to_string() const {
    switch (data.index()) {
        case 0: return "null";
        case 1: return std::to_string(as_int());
        case 2: return std::to_string(as_float());
        case 3: return as_bool() ? "true" : "false";
        case 4: return as_string();
        case 5: return as_object()->to_string();
        default: return "unknown";
    }
}

bool Value::is_truthy() const {
    switch (data.index()) {
        case 0: return false; // null
        case 1: return as_int() != 0;
        case 2: return as_float() != 0;
        case 3: return as_bool();
        case 4: return !as_string().empty();
        case 5: return as_object()->is_truthy();
        default: return false;
    }
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

    if (lhs.is_array() && rhs.is_array()) {
        Array* a1 = lhs.as_array();
        Array* a2 = rhs.as_array();
        if (a1->size() != a2->size()) return false;
        for (size_t i = 0; i < a1->size(); ++i) {
            if ((*a1)[i] != (*a2)[i]) return false;
        }
        return true;
    }
    if (lhs.is_object() && rhs.is_object()) {
        Object* obj1 = lhs.as_object();
        Object* obj2 = rhs.as_object();
        return obj1 == obj2; // compare pointers for non-array objects
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

Value multiply_add(const Value &a, const Value &b, const Value &c) {
    if (a.is_int() && b.is_int() && c.is_int())
        return a.as_int() * b.as_int() + c.as_int();

    if ((a.is_int() || a.is_float()) && (b.is_int() || b.is_float()) && (c.is_int() || c.is_float()))
        return a.as_float() * b.as_float() + c.as_float();

    return (a * b) + c;
}
