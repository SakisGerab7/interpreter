#pragma once

#include "common.hpp"
#include "token.hpp"

struct Callable;
struct Array;
struct Object;
struct Struct;
struct StructInstance;

struct Value {
    using CallablePtr = std::shared_ptr<Callable>;
    using ArrayPtr = std::shared_ptr<Array>;
    using ObjectPtr = std::shared_ptr<Object>;
    using StructPtr = std::shared_ptr<Struct>;
    using StructInstancePtr = std::shared_ptr<StructInstance>;

    using ValueData = std::variant<
        std::monostate,
        int,
        double,
        bool,
        std::string,
        CallablePtr,
        ArrayPtr,
        ObjectPtr,
        StructPtr,
        StructInstancePtr
    >;

    ValueData data;

    Value() : data(std::monostate{}) {}
    Value(int i) : data(i) {}
    Value(double f) : data(f) {}
    Value(bool b) : data(b) {}
    Value(const std::string &s) : data(s) {}
    Value(std::string &&s) : data(std::move(s)) {}
    Value(const char *s) : data(std::string(s)) {}
    Value(CallablePtr c) : data(std::move(c)) {}
    Value(ArrayPtr arr) : data(std::move(arr)) {}
    Value(ObjectPtr obj) : data(std::move(obj)) {}
    Value(StructPtr strct) : data(std::move(strct)) {}
    Value(StructInstancePtr inst) : data(std::move(inst)) {}

    inline bool is_null()     const { return std::holds_alternative<std::monostate>(data); }
    inline bool is_int()      const { return std::holds_alternative<int>(data); }
    inline bool is_float()    const { return std::holds_alternative<double>(data); }
    inline bool is_bool()     const { return std::holds_alternative<bool>(data); }
    inline bool is_string()   const { return std::holds_alternative<std::string>(data); }
    inline bool is_callable() const { return std::holds_alternative<CallablePtr>(data); }
    inline bool is_array()    const { return std::holds_alternative<ArrayPtr>(data); }
    inline bool is_object()   const { return std::holds_alternative<ObjectPtr>(data); }
    inline bool is_struct()   const { return std::holds_alternative<StructPtr>(data); }
    inline bool is_struct_instance() const { return std::holds_alternative<StructInstancePtr>(data); }
    
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
    
    inline const ArrayPtr& as_array() const {
        if (is_array()) return std::get<ArrayPtr>(data);
        throw std::runtime_error("Value is not an array");
    }

    inline const ObjectPtr& as_object() const {
        if (is_object()) return std::get<ObjectPtr>(data);
        throw std::runtime_error("Value is not an object");
    }

    inline const StructPtr& as_struct() const {
        if (is_struct()) return std::get<StructPtr>(data);
        throw std::runtime_error("Value is not a struct");
    }

    inline const StructInstancePtr& as_struct_instance() const {
        if (is_struct_instance()) return std::get<StructInstancePtr>(data);
        throw std::runtime_error("Value is not a struct instance");
    }

    void set_index(int index, const Value &val);
    void set_index(const std::string &key, const Value &val);

    std::string to_string() const;

    bool is_truthy() const;
    operator bool() const { return is_truthy(); }
};

struct Array {
    std::vector<Value> Elements;

    Array(const std::vector<Value> &elems) : Elements(elems) {}

    inline size_t size() const { return Elements.size(); }
    inline bool empty() const { return Elements.empty(); }

    inline Value& operator[](size_t index) {
        if (index >= Elements.size()) {
            Elements.resize(index + 1);
        }
        return Elements[index];
    }

    inline const Value& operator[](size_t index) const {
        return Elements[index];
    }

    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;

    inline iterator begin() { return Elements.begin(); }
    inline iterator end() { return Elements.end(); }
    inline const_iterator begin() const { return Elements.begin(); }
    inline const_iterator end() const { return Elements.end(); }
    inline const_iterator cbegin() const { return Elements.cbegin(); }
    inline const_iterator cend() const { return Elements.cend(); }

    std::string to_string() const;
};

struct Object {
    std::unordered_map<std::string, Value> Items;

    Object(const std::unordered_map<std::string, Value> &map) : Items(map) {}

    inline Value& operator[](const std::string &key) { return Items[key]; }
    inline const Value& operator[](const std::string &key) const { return Items.at(key); }
    
    using iterator = std::unordered_map<std::string, Value>::iterator;
    using const_iterator = std::unordered_map<std::string, Value>::const_iterator;
 
    inline const_iterator find(const std::string &key) const { return Items.find(key); }
    
    inline iterator begin() { return Items.begin(); }
    inline iterator end() { return Items.end(); }
    inline const_iterator begin() const { return Items.begin(); }
    inline const_iterator end() const { return Items.end(); }
    inline const_iterator cbegin() const { return Items.cbegin(); }
    inline const_iterator cend() const { return Items.cend(); }

    std::string to_string() const;
};

struct Struct {
    std::string_view Name;
    std::unordered_map<std::string, Value> Methods;
    
    Struct(std::string_view name, const std::unordered_map<std::string, Value> &methods)
        : Name(name), Methods(methods) {}

    std::string to_string() const { return "<struct " + std::string(Name) + ">"; };
};

struct StructInstance {
    std::shared_ptr<Struct> Strct;
    std::unordered_map<std::string, Value> Fields;

    StructInstance(std::shared_ptr<Struct> strct) : Strct(std::move(strct)) {}

    inline const Value& get(const std::string &name) const {
        auto it = Fields.find(name);
        if (it != Fields.end()) return it->second;

        auto it2 = Strct->Methods.find(name);
        if (it2 != Strct->Methods.end()) return it2->second;

        throw std::runtime_error("Undefined property `" + std::string(name) + "`.");
    }

    inline void put(const std::string &name, const Value &value) {
        Fields[name] = value;
    }

    std::string to_string() const { return "<instance of '" + std::string(Strct->Name) + "'>"; };
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
