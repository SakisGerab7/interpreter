#pragma once

#include "value.hpp"
#include "bytecode.hpp"
#include "serializer.hpp"

// Forward declarations
struct Function;
struct Native;
struct Closure;
struct MethodClosure;
struct Array;
struct Record;
struct ByteArray;
struct Struct;
struct StructInstance;
struct Upvalue;
struct GreenThread;
struct Pipe;
struct IOHandle;

struct VM;

struct Serializer;

struct Object {
    enum class Type {
        Function,
        Native,
        Closure,
        MethodClosure,
        Upvalue,
        Array,
        Record,
        ByteArray,
        Struct,
        StructInstance,
        Thread,
        IOHandle,
        Pipe
    } type;

    size_t id; // for GC and debugging
    bool marked = false; // for GC
    size_t tracked_size = 0; // tracked heap usage for GC thresholds

    Object(Type t) : type(t) {}
    virtual ~Object() = default;

    virtual std::string type_name() const = 0;
    virtual std::string to_string() const = 0;
    virtual bool is_truthy() const = 0;
    virtual void serialize(Serializer& serializer) = 0;
    virtual size_t object_size() const = 0;

    inline bool is_function()        const { return type == Type::Function;       }
    inline bool is_native()          const { return type == Type::Native;         }
    inline bool is_closure()         const { return type == Type::Closure;        }
    inline bool is_method_closure()  const { return type == Type::MethodClosure;  }
    inline bool is_upvalue()         const { return type == Type::Upvalue;        }
    inline bool is_array()           const { return type == Type::Array;          }
    inline bool is_record()          const { return type == Type::Record;         }
    inline bool is_byte_array()      const { return type == Type::ByteArray;      }
    inline bool is_struct()          const { return type == Type::Struct;         }
    inline bool is_struct_instance() const { return type == Type::StructInstance; }
    inline bool is_thread()          const { return type == Type::Thread;         }
    inline bool is_io_handle()       const { return type == Type::IOHandle;       }
    inline bool is_pipe()            const { return type == Type::Pipe;           }

    inline Function*       as_function()        { return reinterpret_cast<Function*>(this);       }
    inline Native*         as_native()          { return reinterpret_cast<Native*>(this);         }
    inline Closure*        as_closure()         { return reinterpret_cast<Closure*>(this);        }
    inline MethodClosure*  as_method_closure()  { return reinterpret_cast<MethodClosure*>(this);  }
    inline Upvalue*        as_upvalue()         { return reinterpret_cast<Upvalue*>(this);        }
    inline Array*          as_array()           { return reinterpret_cast<Array*>(this);          }
    inline Record*         as_record()          { return reinterpret_cast<Record*>(this);         }
    inline ByteArray*      as_byte_array()      { return reinterpret_cast<ByteArray*>(this);      }
    inline Struct*         as_struct()          { return reinterpret_cast<Struct*>(this);         }
    inline StructInstance* as_struct_instance() { return reinterpret_cast<StructInstance*>(this); }
    inline GreenThread*    as_thread()          { return reinterpret_cast<GreenThread*>(this);    }
    inline IOHandle*       as_io_handle()       { return reinterpret_cast<IOHandle*>(this);       }
    inline Pipe*           as_pipe()            { return reinterpret_cast<Pipe*>(this);           }
};

// Function pointer type for native functions
using NativeFn = std::function<Value(VM&, const std::vector<Value> &)>;

struct Function : public Object {
    std::string name;
    Chunk chunk;
    int arity;
    int upvalue_count = 0;

    Function(const std::string &name, int arity)
        : Object(Type::Function), name(name), arity(arity) {}

    std::string type_name() const override { return "Function"; }
    std::string to_string() const override { return "<fn " + name + "/" + std::to_string(arity) + ">"; }
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_function(this); }
    size_t object_size() const override {
        size_t size = sizeof(Function);
        size += name.capacity() * sizeof(char);
        size += chunk.code.capacity() * sizeof(uint8_t);
        size += chunk.constants.capacity() * sizeof(Value);
        return size;
    }
};

struct Native : public Object {
    std::string name;
    int arity;
    NativeFn func;
    Value bound_instance; // for methods

    Native(const std::string &name, int arity, NativeFn func)
        : Object(Type::Native), name(name), arity(arity), func(std::move(func)) {}

    std::string type_name() const override { return "Native"; }
    std::string to_string() const override { return "<fn " + name + "/" + std::to_string(arity) + ">"; }
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_native(this); }
    size_t object_size() const override {
        size_t size = sizeof(Native);
        size += name.capacity() * sizeof(char);
        return size;
    }
};

struct Upvalue : public Object {
    GreenThread* owner_thread; // nullptr if closed
    int slot_index = -1;
    Value closed;

    Upvalue(GreenThread* thread, int slot)
        : Object(Type::Upvalue), owner_thread(thread), slot_index(slot), closed() {}

    std::string type_name() const override { return "Upvalue"; }
    std::string to_string() const override { return get().to_string(); }
    bool is_truthy() const override { return get().is_truthy(); }
    void serialize(Serializer& serializer) override { serializer.print_upvalue(this); }
    size_t object_size() const override { return sizeof(Upvalue); }

    Value get() const;
    void set(const Value &v);
};

struct Closure : public Object {
    Function* func;
    std::vector<Upvalue*> upvalues;
    int upvalue_count;

    Closure(Function* f)
        : Object(Type::Closure), func(std::move(f))
    {
        upvalue_count = func->upvalue_count;
        upvalues.reserve(upvalue_count);
    }

    std::string type_name() const override { return "Closure"; }
    std::string to_string() const override { return func->to_string(); }
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_closure(this); }
    size_t object_size() const override {
        size_t size = sizeof(Closure);
        size += upvalues.capacity() * sizeof(Upvalue*);
        return size;
    }
};

struct MethodClosure : public Object {
    Closure* closure;
    Value self;

    MethodClosure(Closure* c, Value s)
        : Object(Type::MethodClosure), closure(std::move(c)), self(std::move(s)) {}

    std::string type_name() const override { return "MethodClosure"; }
    std::string to_string() const override { return closure->to_string(); }
    bool is_truthy() const override { return closure->is_truthy(); }
    void serialize(Serializer& serializer) override { serializer.print_method_closure(this); }
    size_t object_size() const override { return sizeof(MethodClosure); }
};

struct Array : public Object {
    std::vector<Value> elements;

    Array() : Object(Type::Array) {}
    Array(const std::vector<Value> &elements)
        : Object(Type::Array), elements(elements) {}

    inline size_t size() const { return elements.size(); }
    inline bool empty() const { return elements.empty(); }

    inline Value& operator[](size_t index) { return elements[index]; }
    inline const Value& operator[](size_t index) const { return elements[index]; }

    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;

    inline iterator begin() { return elements.begin(); }
    inline iterator end() { return elements.end(); }
    inline const_iterator begin() const { return elements.begin(); }
    inline const_iterator end() const { return elements.end(); }
    inline const_iterator cbegin() const { return elements.cbegin(); }
    inline const_iterator cend() const { return elements.cend(); }

    std::string type_name() const override { return "Array"; }
    std::string to_string() const override;
    bool is_truthy() const override { return !empty(); }
    void serialize(Serializer& serializer) override { serializer.print_array(this); }
    size_t object_size() const override {
        size_t size = sizeof(Array);
        size += elements.capacity() * sizeof(Value);
        return size;
    }
};

struct Record : public Object {
    std::unordered_map<std::string, Value> items;

    Record() : Object(Type::Record) {}
    Record(const std::unordered_map<std::string, Value> &map) : Object(Type::Record), items(map) {}

    inline size_t size() const { return items.size(); }
    inline bool empty() const { return items.empty(); }

    inline Value& operator[](const std::string &key) { return items[key]; }
    inline const Value& operator[](const std::string &key) const { return items.at(key); }

    using iterator = std::unordered_map<std::string, Value>::iterator;
    using const_iterator = std::unordered_map<std::string, Value>::const_iterator;

    inline const_iterator find(const std::string &key) const { return items.find(key); }

    inline iterator begin() { return items.begin(); }
    inline iterator end() { return items.end(); }
    inline const_iterator begin() const { return items.begin(); }
    inline const_iterator end() const { return items.end(); }
    inline const_iterator cbegin() const { return items.cbegin(); }
    inline const_iterator cend() const { return items.cend(); }

    std::string type_name() const override { return "Record"; }
    std::string to_string() const override;
    bool is_truthy() const override { return !empty(); }
    void serialize(Serializer& serializer) override { serializer.print_record(this); }
    size_t object_size() const override {
        size_t size = sizeof(Record);
        size += items.bucket_count() * sizeof(void*);
        size += items.size() * sizeof(std::pair<const std::string, Value>);
        for (const auto &pair : items) {
            size += pair.first.capacity() * sizeof(char);
        }
        return size;
    }
};

struct ByteArray : public Object {
    std::vector<uint8_t> data;

    ByteArray() : Object(Type::ByteArray) {}
    ByteArray(const std::vector<uint8_t> &data)
        : Object(Type::ByteArray), data(data) {}

    inline size_t size() const { return data.size(); }
    inline bool empty() const { return data.empty(); }

    inline uint8_t& operator[](size_t index) { return data[index]; }
    inline const uint8_t& operator[](size_t index) const { return data[index]; }

    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;

    inline iterator begin() { return data.begin(); }
    inline iterator end() { return data.end(); }
    inline const_iterator begin() const { return data.begin(); }
    inline const_iterator end() const { return data.end(); }
    inline const_iterator cbegin() const { return data.cbegin(); }
    inline const_iterator cend() const { return data.cend(); }

    std::string type_name() const override { return "ByteArray"; }
    std::string to_string() const override;
    bool is_truthy() const override { return !empty(); }
    void serialize(Serializer& serializer) override { serializer.print_byte_array(this); }
    size_t object_size() const override {
        size_t size = sizeof(ByteArray);
        size += data.capacity() * sizeof(uint8_t);
        return size;
    }
};

struct Struct : public Object {
    std::string name;
    std::unordered_map<std::string, Value> methods;

    Struct(const std::string &name)
        : Object(Type::Struct), name(name) {}

    inline void add_method(const std::string &name, const Value &method) {
        methods[name] = method;
    }

    std::string type_name() const override { return "Struct"; }
    std::string to_string() const override { return "<struct " + name + ">"; };
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_struct(this); }
    size_t object_size() const override {
        size_t size = sizeof(Struct);
        size += name.capacity() * sizeof(char);
        size += methods.bucket_count() * sizeof(void*);
        size += methods.size() * sizeof(std::pair<const std::string, Value>);
        for (const auto &pair : methods) {
            size += pair.first.capacity() * sizeof(char);
        }
        return size;
    }
};

struct StructInstance : public Object {
    Struct* struct_ptr;
    std::unordered_map<std::string, Value> fields;

    StructInstance(Struct* strct)
        : Object(Type::StructInstance), struct_ptr(std::move(strct)) {}

    inline const Value& get(const std::string &name) const {
        if (auto it = fields.find(name); it != fields.end()) return it->second;
        if (auto it2 = struct_ptr->methods.find(name); it2 != struct_ptr->methods.end()) return it2->second;
        throw std::runtime_error("Undefined property `" + std::string(name) + "`.");
    }

    inline void put(const std::string &name, const Value &value) {
        fields[name] = value;
    }

    std::string type_name() const override { return "StructInstance"; }
    std::string to_string() const override { return "<instance of '" + std::string(struct_ptr->name) + "'>"; };
    bool is_truthy() const override { return true; }
    void serialize(Serializer& serializer) override { serializer.print_struct_instance(this); }
    size_t object_size() const override {
        size_t size = sizeof(StructInstance);
        size += fields.bucket_count() * sizeof(void*);
        size += fields.size() * sizeof(std::pair<const std::string, Value>);
        for (const auto &pair : fields) {
            size += pair.first.capacity() * sizeof(char);
        }
        return size;
    }
};
