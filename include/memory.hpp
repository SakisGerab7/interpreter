#pragma once

#include "common.hpp"
#include "runtime.hpp"

struct VM;

struct Heap {
    std::vector<Object*> objects;
    std::vector<Object*> gray_stack;

    size_t next_id = 1;

    size_t total_bytes = 0;
    size_t next_gc_threshold = 32 * 1024; // 1KB initial threshold

    bool debug = true;

    VM* active_vm = nullptr;

    Heap() {
        gray_stack.reserve(1024);
    }

    ~Heap();

    template <typename T, typename... Args>
    inline T* allocate(Args&&... args) {
        if (active_vm && total_bytes >= next_gc_threshold) {
            collect_garbage();
        }

        T* obj = new T(std::forward<Args>(args)...);
        obj->id = next_id++;
        obj->tracked_size = obj->object_size();

        total_bytes += obj->tracked_size;
        objects.push_back(obj);

        log_allocation(obj);
        return obj;
    }

    void log_allocation(const Object* obj);
    void log_deallocation(const Object* obj);

    void collect_garbage();

    void mark_roots();
    void mark_value(const Value &val);
    void mark_object(Object* obj);

    void trace_references();
    void blacken_object(Object* obj);

    void sweep();
};
