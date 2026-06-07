#include "memory.hpp"
#include "vm.hpp"

Heap::~Heap() {
    for (Object* obj : objects) {
        log_deallocation(obj);
        delete obj;
    }
}

void Heap::log_allocation(const Object* obj) {
    if (debug) {
        std::cerr << "[Heap] Allocating " << obj->type_name() << " at " << obj << "\n";
    }
}

void Heap::log_deallocation(const Object* obj) {
    if (debug) {
        std::cerr << "[Heap] Deallocating " << obj->type_name() << " at " << obj << "\n";
    }
}

void Heap::collect_garbage() {
    if (debug) {
        std::cerr << "[GC] Starting garbage collection. Current allocations: " << objects.size()
                    << ", total bytes: " << total_bytes
                    << ", next GC threshold: " << next_gc_threshold << "\n";
    }

    size_t before_count = objects.size();

    mark_roots();
    trace_references();
    sweep();

    next_gc_threshold = total_bytes + (total_bytes / 2); // increase threshold by 50% to reduce frequency of GC

    if (debug) {
        std::cerr << "[GC] Finished garbage collection. Freed " << (before_count - objects.size()) << " objects, "
                  << "total bytes: " << total_bytes
                  << ", next GC threshold: " << next_gc_threshold << "\n";
    }
}

void Heap::mark_roots() {
    // Mark main thread
    if (active_vm->main_thread) {
        mark_object(active_vm->main_thread);
    }

    // Mark current thread
    if (active_vm->current_thread) {
        mark_object(active_vm->current_thread);
    }

    // Mark cli arguments
    mark_value(active_vm->cli_arguments);

    // Mark global variables
    for (const auto &global : active_vm->globals) {
        mark_value(global.second);
    }
}

void Heap::trace_references() {
    while (!gray_stack.empty()) {
        Object* obj = gray_stack.back();
        gray_stack.pop_back();
        blacken_object(obj);
    }
}

void Heap::sweep() {
    size_t dst = 0;

    for (size_t src = 0; src < objects.size(); src++) {
        Object* obj = objects[src];

        if (obj->marked) {
            obj->marked = false;
            objects[dst++] = obj;
        } else {
            log_deallocation(obj);
            total_bytes -= obj->tracked_size;
            delete obj;
        }
    }

    objects.resize(dst);
}

void Heap::mark_value(const Value &val) {
    if (val.is_object()) {
        mark_object(val.as_object());
    }
}

void Heap::mark_object(Object* obj) {
    if (obj->is_thread()) {
        GreenThread* thread = obj->as_thread();
        if (thread->state == GreenThread::Finished && thread->detached) {
            return; // skip finished detached threads
        }
    }

    if (obj->marked) return; // already marked
    obj->marked = true;
    gray_stack.push_back(obj);
}

void Heap::blacken_object(Object* obj) {
    switch (obj->type) {
        case Object::Type::Function: {
            auto func = obj->as_function();
            for (const Value &constant : func->chunk.constants) {
                mark_value(constant);
            }
            break;
        }
        case Object::Type::Native: {
            auto native = obj->as_native();
            mark_value(native->bound_instance);
            break;
        }
        case Object::Type::Closure: {
            auto closure = obj->as_closure();
            mark_object(closure->func);
            for (Upvalue* upvalue : closure->upvalues) {
                mark_object(upvalue);
            }
            break;
        }
        case Object::Type::MethodClosure: {
            auto method_closure = obj->as_method_closure();
            mark_object(method_closure->closure);
            mark_value(method_closure->self);
            break;
        }
        case Object::Type::Upvalue: {
            auto upvalue = obj->as_upvalue();
            mark_value(upvalue->get());
            break;
        }
        case Object::Type::Array: {
            auto arr = obj->as_array();
            for (const Value &elem : arr->elements) {
                mark_value(elem);
            }
            break;
        }
        case Object::Type::Record: {
            auto record = obj->as_record();
            for (const auto &field : *record) {
                mark_value(field.second);
            }
            break;
        }
        case Object::Type::ByteArray: {
            break;
        }
        case Object::Type::Struct: {
            auto strct = obj->as_struct();
            for (const auto &method : strct->methods) {
                mark_value(method.second);
            }
            break;
        }
        case Object::Type::StructInstance: {
            auto instance = obj->as_struct_instance();
            mark_object(instance->struct_ptr);
            for (const auto &field : instance->fields) {
                mark_value(field.second);
            }
            break;
        }
        case Object::Type::Thread: {
            auto thread = obj->as_thread();
            for (size_t i = 0; i < thread->ctx.stack_size; i++) {
                mark_value(thread->ctx.stack[i]);
            }
            for (const CallFrame &frame : thread->ctx.frames) {
                mark_object(frame.closure);
            }
            for (Upvalue* upvalue : thread->ctx.open_upvalues) {
                mark_object(upvalue);
            }
            for (GreenThread* joiner : thread->joiners) {
                mark_object(joiner);
            }
            for (GreenThread* child : thread->children) {
                mark_object(child);
            }
            mark_value(thread->pending_value);
            if (thread->active_select) {
                for (const auto &select_case : thread->active_select->cases) {
                    mark_object(select_case.pipe);
                    if (select_case.type == SelectCase::Type::Send) {
                        mark_value(select_case.value);
                    }
                }
            }
            break;
        }
        case Object::Type::IOHandle: {
            auto handle = obj->as_io_handle();
            for (const auto &pending_read : handle->reads) {
                mark_object(pending_read.thread);
            }
            for (const auto &pending_write : handle->writes) {
                mark_object(pending_write.thread);
            }
            for (const auto &pending_accept : handle->accepts) {
                mark_object(pending_accept.thread);
            }
            for (const auto &pending_connect : handle->connects) {
                mark_object(pending_connect.thread);
            }
            break;
        }
        case Object::Type::Pipe: {
            auto pipe = obj->as_pipe();
            for (const Value &val : pipe->buffer) {
                mark_value(val);
            }
            for (auto reader : pipe->readers) {
                mark_object(reader);
            }
            for (auto writer : pipe->writers) {
                mark_object(writer);
            }
            for (auto selector : pipe->selectors) {
                mark_object(selector);
            }
            break;
        }
        default:
            // No references to trace.
            break;
    }
}
