#include "vm.hpp"
#include "bytecode.hpp"
#include "native_functions.hpp"
#include "runtime.hpp"
#include "value.hpp"
#include <cstdint>

#define PROFILING_ENABLED false

struct OpTimer {
    VM &vm;
    uint8_t op;
    std::chrono::steady_clock::time_point start;

    OpTimer(VM &vm_, uint8_t op_)
        : vm(vm_), op(op_), start(std::chrono::steady_clock::now()) {}

    ~OpTimer() {
        auto end = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        if (ns < 0) ns = 0;
        vm.opcode_time_ns[op] += static_cast<uint64_t>(ns);
        vm.total_time_ns += static_cast<uint64_t>(ns);
    }
};

VM::VM(const std::vector<std::string> &args, Heap* heap_ptr) : scheduler(*this), heap(heap_ptr) {
    std::vector<Value> arg_values;
    for (const auto &arg : args) {
        arg_values.push_back(arg);
    }

    cli_arguments = heap->allocate<Array>(arg_values);

    // define native functions here if needed
    define_native("cli_args", 0, native_functions::cli_args);

    define_native("clock", 0, native_functions::clock);
    define_native("len",   1, native_functions::len);
    define_native("str",   1, native_functions::str);
    define_native("int",   1, native_functions::int_fn);
    define_native("float", 1, native_functions::float_fn);
    define_native("type",  1, native_functions::type);

    define_native("String.upper", 0, native_functions::string::to_upper);
    define_native("String.lower", 0, native_functions::string::to_lower);
    define_native("String.trim",  0, native_functions::string::trim);
    define_native("String.split", 1, native_functions::string::split);
    define_native("String.bytes", 0, native_functions::string::to_byte_array);

    define_native("ByteArray.str", 0, native_functions::byte_array::to_string);

    define_native("arange", 3, native_functions::array::arange);
    define_native("Array.push",    1, native_functions::array::push);
    define_native("Array.pop",     0, native_functions::array::pop);
    define_native("Array.shift",   0, native_functions::array::shift);
    define_native("Array.unshift", 1, native_functions::array::unshift);
    define_native("Array.slice",   2, native_functions::array::slice);
    define_native("Array.sum",     0, native_functions::array::sum);

    define_native("Array.for_each", 1, native_functions::array::foreach);
    define_native("Array.map",      1, native_functions::array::map);

    globals["pi"] = M_PI;
    define_native("pow",     2, native_functions::math::pow);
    define_native("abs",     1, native_functions::math::abs);
    define_native("round",   1, native_functions::math::round);
    define_native("sqrt",    1, native_functions::math::sqrt);
    define_native("sin",     1, native_functions::math::sin);
    define_native("cos",     1, native_functions::math::cos);
    define_native("tan",     1, native_functions::math::tan);
    define_native("floor",   1, native_functions::math::floor);
    define_native("ceil",    1, native_functions::math::ceil);
    define_native("min",     2, native_functions::math::min);
    define_native("max",     2, native_functions::math::max);
    define_native("srand",   1, native_functions::math::srand);
    define_native("rand",    0, native_functions::math::rand);
    define_native("randint", 2, native_functions::math::randint);
    define_native("asin",    1, native_functions::math::asin);
    define_native("acos",    1, native_functions::math::acos);
    define_native("atan",    1, native_functions::math::atan);
    define_native("log2",    1, native_functions::math::log2);
    define_native("log10",   1, native_functions::math::log10);
    define_native("ln",      1, native_functions::math::ln);
    define_native("exp",     1, native_functions::math::exp);

    define_native("sleep",     1, native_functions::sleep);
    define_native("thread_id", 0, native_functions::thread_id);

    define_native("Thread.join", 0, native_functions::join);
    define_native("Thread.detach", 0, native_functions::detach);

    define_native("pipe", 1, native_functions::pipe);

    // I/O related globals
    globals["stdin"] = scheduler.stdin_handle();
    globals["stdout"] = scheduler.stdout_handle();
    globals["stderr"] = heap->allocate<IOHandle>();

    // I/O related natives
    define_native("listen",  2, native_functions::listen);
    define_native("connect", 2, native_functions::connect);
    define_native("open",    2, native_functions::open);

    define_native("IOHandle.read", 1, native_functions::io::read);
    define_native("IOHandle.read_until_delimiter", 1, native_functions::io::read_until_delimiter);
    define_native("IOHandle.read_line", 0, native_functions::io::read_line);
    define_native("IOHandle.read_all", 0, native_functions::io::read_all);
    define_native("IOHandle.write", 1, native_functions::io::write);
    define_native("IOHandle.accept", 0, native_functions::io::accept);
    define_native("IOHandle.close_", 0, native_functions::io::close);

    define_native("pack",   2, native_functions::pack);
    define_native("unpack", 2, native_functions::unpack);
}

void VM::spawn_thread(Closure* closure, size_t thread_count) {
    std::vector<Value> handles;
    for (size_t i = 0; i < thread_count; ++i) {
        GreenThread* new_thread = heap->allocate<GreenThread>(scheduler.next_thread_id++, closure);

        scheduler.enqueue(new_thread);

        if (current_thread) {
            new_thread->parent = current_thread;
            current_thread->children.push_back(new_thread);
        }

        handles.push_back(new_thread);
    }

    if (current_thread) {
        push((thread_count == 1) ? handles[0] : heap->allocate<Array>(handles));
    } else {
        // If this is the first thread, set it as the main thread
        main_thread = handles[0].as_thread();
    }
}

Value VM::interpret(Function* func) {
    Closure* closure = heap->allocate<Closure>(func);
    spawn_thread(closure, 1);

    std::cerr << "Starting VM with main thread ID " << main_thread->ID << " (state: " << main_thread->state << ")\n";

    heap->active_vm = this; // set active VM for GC

    Value result = scheduler.schedule();

    std::cerr << "VM execution finished. Result: " << result.to_string() << "\n";

#if PROFILING_ENABLED
    dump_profile();
    profile_dumped = true;
#endif

    return result;
}

void VM::define_native(const std::string &name, int arity, NativeFn func) {
    globals[name] = heap->allocate<Native>(name, arity, func);
}

void VM::bind_native_method(const Value &obj, const std::string &method_name) {
    auto method_it = globals.find(method_name);
    if (method_it == globals.end()) {
        throw std::runtime_error("Undefined method '" + method_name + "'");
    }

    auto method = method_it->second.as_native();
    method->bound_instance = obj;
    push(method);
}


void VM::call_value(const Value &callee, int arg_count) {
    if (callee.is_closure()) {
        call(callee.as_closure(), arg_count);
        return;
    }
    if (callee.is_function()) {
        auto func = callee.as_function();
        auto closure = heap->allocate<Closure>(func);
        call(closure, arg_count);
        return;
    }
    if (callee.is_method_closure()) {
        auto closure = callee.as_method_closure();
        poke(closure->self, arg_count); // set 'self' as the first argument
        call(closure->closure, arg_count);
        return;
    }
    if (callee.is_native()) {
        call_native(callee.as_native(), arg_count);
        return;
    }
    if (callee.is_struct()) {
        // Creating a new instance of the struct
        auto strct = callee.as_struct();
        poke(heap->allocate<StructInstance>(strct), arg_count);

        auto it = strct->methods.find("init");
        if (it != strct->methods.end()) {
            auto init_method = it->second;
            call_value(init_method, arg_count);
            return;
        } else if (arg_count != 0) {
            throw std::runtime_error("Struct constructor does not take arguments");
        }
        return;
    }

    throw std::runtime_error("Value is not callable");
}

void VM::call_native(Native* native, int arg_count) {
    if (arg_count != native->arity) {
        throw std::runtime_error("Expected " + std::to_string(native->arity) +
                                 " arguments but got " + std::to_string(arg_count));
    }

    std::vector<Value> args;
    if (!native->bound_instance.is_null()) {
        // If this is a method call, set the first argument to bound_instance
        args.push_back(native->bound_instance);
        native->bound_instance = {}; // clear after use
    }

    for (int i = arg_count - 1; i >= 0; i--) {
        args.push_back(peek(i));
    }

    for (int i = 0; i < arg_count; i++) {
        pop();
    }
    pop(); // pop the native function itself

    Value result = native->func(*this, args);
    push(result);
}

void VM::call(Closure* closure, int arg_count) {
    if (arg_count != closure->func->arity) {
        throw std::runtime_error("Expected " + std::to_string(closure->func->arity) +
                                 " arguments but got " + std::to_string(arg_count));
    }

    if (current_thread->ctx.frames.size() >= 256) {
        throw std::runtime_error("Stack overflow");
    }

    current_thread->ctx.frames.emplace_back(closure, 0, static_cast<int>(current_thread->ctx.stack_size - arg_count - 1));
}

Value Upvalue::get() const {
    if (owner_thread && slot_index >= 0) {
        return owner_thread->ctx.stack[static_cast<size_t>(slot_index)];
    }

    return closed;
}

void Upvalue::set(const Value &v) {
    if (owner_thread && slot_index >= 0) {
        owner_thread->ctx.stack[static_cast<size_t>(slot_index)] = v;
        return;
    }

    closed = v;
}

Upvalue* VM::capture_upvalue(int slot_index) {
    // Check if we already have an open upvalue pointing to this stack slot.
    for (auto &uv : current_thread->ctx.open_upvalues) {
        if (uv->owner_thread == current_thread && uv->slot_index == slot_index)
            return uv;
    }

    // Otherwise create a new upvalue for this local
    auto up = heap->allocate<Upvalue>(current_thread, slot_index);
    current_thread->ctx.open_upvalues.push_back(up);
    return up;
}

void VM::close_upvalues(int last) {
    for (auto it = current_thread->ctx.open_upvalues.begin(); it != current_thread->ctx.open_upvalues.end(); ) {
        auto upvalue = *it;
        if (upvalue->owner_thread == current_thread && upvalue->slot_index >= last) {
            // Move the value from the stack to the upvalue's closed field
            upvalue->closed = current_thread->ctx.stack[static_cast<size_t>(upvalue->slot_index)];
            upvalue->owner_thread = {};
            upvalue->slot_index = -1;
            it = current_thread->ctx.open_upvalues.erase(it);
        } else {
            ++it;
        }
    }
}

uint8_t VM::read_byte(CallFrame &frame) {
    return frame.closure->func->chunk.code[frame.ip++];
}

uint16_t VM::read_short(CallFrame &frame) {
    uint16_t high = read_byte(frame);
    uint16_t low = read_byte(frame);
    return (high << 8) | low;
}

void VM::push(const Value& v) {
    if (current_thread->ctx.stack_size >= current_thread->ctx.stack.size()) {
        throw std::runtime_error("Stack overflow");
    }

    current_thread->ctx.stack[current_thread->ctx.stack_size++] = v;
}

const Value &VM::pop() const {
    if (current_thread->ctx.stack_size == 0) {
        throw std::runtime_error("Stack underflow");
    }

    return current_thread->ctx.stack[--current_thread->ctx.stack_size];
}

const Value& VM::peek(size_t depth) const {
    return current_thread->ctx.peek_stack(depth);
}

void VM::poke(const Value &v, size_t depth) {
    current_thread->ctx.poke_stack(v, depth);
}

void VM::debug_instruction(CallFrame &frame, OpCode op) {
    std::cerr << "[Thread " << current_thread->ID << "] ";
    std::cerr << "[IP " << std::hex << std::right << std::setw(4)  << std::setfill('0') << (frame.ip - 1)
              << "] "   << std::dec << std::left  << std::setw(15) << std::setfill(' ') << opcode_to_string(op)
              << " | ";

    std::cerr << "Stack: [";
    for (size_t i = 0; i < current_thread->ctx.stack_size; ++i) {
        std::cerr << current_thread->ctx.stack[i].to_string();
        if (i < current_thread->ctx.stack_size - 1) std::cerr << ", ";
    }

    std::cerr << "]\n";
}

std::string VM::stack_trace() const {
    std::string trace;
    for (auto it = current_thread->ctx.frames.rbegin(); it != current_thread->ctx.frames.rend(); ++it) {
        auto &frame = *it;
        trace += "  at " + frame.closure->func->to_string() + "\n";
    }

    return trace;
}

void VM::run() {
    while (true) {
        if (vm_signal::suspend_requested) [[unlikely]] return;

        if (current_thread->ctx.frames.empty()) [[unlikely]] {
            current_thread->state = GreenThread::Finished;
            return;
        }

        CallFrame &current_frame = current_thread->ctx.frames.back();
        auto &chunk = current_frame.closure->func->chunk;
        if (current_frame.ip >= chunk.code.size()) [[unlikely]] {
            current_thread->ctx.frames.pop_back();
            if (current_thread->ctx.frames.empty()) [[unlikely]] {
                current_thread->state = GreenThread::Finished;
                return;
            }
            continue;
        }

        OpCode op = static_cast<OpCode>(read_byte(current_frame));

#if PROFILING_ENABLED
        opcode_counts[static_cast<uint8_t>(op)]++;
        total_instructions++;

        OpTimer timer(*this, static_cast<uint8_t>(op));
#endif

        debug_instruction(current_frame, op);

        switch (op) {
            case OP_NULL:  push({});    break;
            case OP_TRUE:  push(true);  break;
            case OP_FALSE: push(false); break;
            case OP_CONST: {
                int idx = read_short(current_frame);
                if (idx >= chunk.constants.size()) throw std::runtime_error("constant index out of range");
                push(chunk.constants[idx]);
                break;
            }
            case OP_ICONST8: {
                int val = static_cast<int8_t>(read_byte(current_frame));
                push(val);
                break;
            }
            case OP_ICONST16: {
                int val = static_cast<int16_t>(read_short(current_frame));
                push(val);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint16_t idx = read_short(current_frame);
                auto name = chunk.constants[idx].as_string();
                globals[name] = pop();
                break;
            }
            case OP_LOAD_GLOBAL: {
                uint16_t idx = read_short(current_frame);
                auto name = chunk.constants[idx].as_string();
                auto it = globals.find(name);
                if (it == globals.end()) throw std::runtime_error("Undefined global variable: " + name);
                push(it->second);
                break;
            }
            case OP_STORE_GLOBAL: {
                uint16_t idx = read_short(current_frame);
                auto name = chunk.constants[idx].as_string();
                auto it = globals.find(name);
                if (it == globals.end()) throw std::runtime_error("Undefined global variable: " + name);
                it->second = peek(0);
                break;
            }
            case OP_LOAD_LOCAL: {
                uint8_t local_idx = read_byte(current_frame);
                int stack_idx = current_frame.base + local_idx;
                if (stack_idx < 0 || stack_idx >= current_thread->ctx.stack_size) {
                    throw std::runtime_error("Local variable index out of range");
                }

                push(current_thread->ctx.stack[stack_idx]);
                break;
            }
            case OP_STORE_LOCAL: {
                uint8_t local_idx = read_byte(current_frame);
                int stack_idx = current_frame.base + local_idx;
                if (stack_idx < 0 || stack_idx >= current_thread->ctx.stack_size) {
                    throw std::runtime_error("Local variable index out of range");
                }

                current_thread->ctx.stack[stack_idx] = peek(0);
                break;
            }
            case OP_LOAD_UPVALUE: {
                uint8_t upvalue_idx = read_byte(current_frame);
                if (upvalue_idx >= current_frame.closure->upvalues.size()) {
                    throw std::runtime_error("Upvalue index out of range");
                }

                auto upvalue = current_frame.closure->upvalues[upvalue_idx];
                push(upvalue->get());
                break;
            }
            case OP_STORE_UPVALUE: {
                uint8_t upvalue_idx = read_byte(current_frame);
                if (upvalue_idx >= current_frame.closure->upvalues.size()) {
                    throw std::runtime_error("Upvalue index out of range");
                }

                auto upvalue = current_frame.closure->upvalues[upvalue_idx];
                upvalue->set(peek(0));
                break;
            }
            case OP_LOAD_FIELD: {
                int idx = read_short(current_frame);
                std::string key = chunk.constants[idx].as_string();
                Value obj = pop();

                if (obj.is_string()) {
                    bind_native_method(obj, "String." + key);
                } else if (obj.is_array()) {
                    bind_native_method(obj, "Array." + key);
                } else if (obj.is_byte_array()) {
                    bind_native_method(obj, "ByteArray." + key);
                } else if (obj.is_thread()) {
                    bind_native_method(obj, "Thread." + key);
                } else if (obj.is_io_handle()) {
                    if (key == "fd") {
                        push(obj.as_io_handle()->fd);
                    } else if (key == "local_addr") {
                        push(obj.as_io_handle()->metadata.local_address);
                    } else if (key == "remote_addr") {
                        push(obj.as_io_handle()->metadata.remote_address);
                    } else {
                        bind_native_method(obj, "IOHandle." + key);
                    }
                } else {
                    auto field_val = obj.get_index(key);
                    if (obj.is_struct_instance() && field_val.is_closure()) {
                        // If it's a struct instance and the field is a closure, we need to create a method closure
                        auto closure = field_val.as_closure();
                        auto method_closure = heap->allocate<MethodClosure>(closure, obj);
                        field_val = method_closure;
                    }

                    push(field_val);
                }

                break;
            }
            case OP_STORE_FIELD: {
                int idx = read_short(current_frame);
                std::string key = chunk.constants[idx].as_string();
                Value val = pop();
                Value obj = pop();
                obj.set_index(key, val);
                push(val);
                break;
            }
            case OP_LOAD_INDEX: {
                Value index = pop();
                Value container = pop();
                try {
                    push(container.get_index(index));
                } catch (const std::exception &e) {
                    throw RuntimeError(*this, std::string("Indexing error: ") + e.what());
                }
                break;
            }
            case OP_STORE_INDEX: {
                Value val = pop();
                Value index = pop();
                Value container = pop();
                container.set_index(index, val);
                push(val);
                break;
            }
            case OP_CLOSURE: {
                uint16_t func_idx = read_short(current_frame);
                if (func_idx >= chunk.constants.size()) {
                    throw std::runtime_error("Function index out of range");
                }

                auto func_val = chunk.constants[func_idx];
                if (!func_val.is_function()) {
                    throw std::runtime_error("Expected function for CLOSURE opcode");
                }

                auto func = func_val.as_function();
                auto closure = heap->allocate<Closure>(func);

                // capture upvalues
                for (int i = 0; i < func->upvalue_count; i++) {
                    uint8_t is_local = read_byte(current_frame);
                    uint8_t index = read_byte(current_frame);
                    if (is_local) {
                        closure->upvalues.push_back(capture_upvalue(current_frame.base + index));
                    } else {
                        closure->upvalues.push_back(current_frame.closure->upvalues[index]);
                    }
                }

                push(closure);
                break;
            }
            case OP_RETURN: {
                Value ret_val = pop();
                close_upvalues(current_frame.base);
                current_thread->ctx.frames.pop_back();
                if (current_thread->ctx.frames.empty()) {
                    current_thread->state = GreenThread::Finished;
                    current_thread->return_value = ret_val;
                    return;
                }

                current_thread->ctx.stack_size = current_frame.base;
                push(ret_val);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(static_cast<int>(current_thread->ctx.stack_size) - 1);
                pop();
                break;
            }
            case OP_POP: {
                pop();
                break;
            }
            case OP_PRINT: {
                auto value = pop();
                std::cout << value.to_string() << std::endl;
                break;
            }
            case OP_DUP: {
                Value v = peek(0);
                push(v);
                break;
            }
            case OP_DUP2: {
                Value a = peek(1);
                Value b = peek(0);
                push(a);
                push(b);
                break;
            }
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_MOD:
            case OP_EQ:
            case OP_NEQ:
            case OP_LT:
            case OP_LE:
            case OP_GT:
            case OP_GE:
            case OP_BIT_AND:
            case OP_BIT_OR:
            case OP_BIT_XOR:
            case OP_SHIFT_LEFT:
            case OP_SHIFT_RIGHT: {
                binary_op(op);
                break;
            }
            case OP_MUL_ADD: {
                Value c = pop();
                Value b = pop();
                Value a = pop();
                push(multiply_add(a, b, c));
                break;
            }
            case OP_SEND_PIPE: {
                Value val = pop();
                Value sender_val = pop();
                if (!sender_val.is_pipe()) throw std::runtime_error("Expected a pipe for SEND_PIPE");

                auto pipe = sender_val.as_pipe();
                if (!pipe) throw std::runtime_error("Invalid pipe in SEND_PIPE");

                pipe->send(val, *this);
                push(val);
                break;
            }
            case OP_RECV_PIPE: {
                Value receiver_val = pop();
                if (!receiver_val.is_pipe()) throw std::runtime_error("Expected a pipe for RECV_PIPE");

                auto pipe = receiver_val.as_pipe();
                if (!pipe) throw std::runtime_error("Invalid pipe in RECV_PIPE");

                Value val = pipe->recv(*this);
                push(val);
                break;
            }
            case OP_CLOSE_PIPE: {
                Value closing_val = pop();
                if (!closing_val.is_pipe()) throw std::runtime_error("Expected a pipe for CLOSE_PIPE");

                auto pipe = closing_val.as_pipe();
                if (!pipe) throw std::runtime_error("Invalid pipe in CLOSE_PIPE");

                pipe->close(*this);
                break;
            }
            case OP_SELECT_BEGIN: {
                uint8_t case_count = read_byte(current_frame);
                current_thread->active_select = std::make_unique<SelectFrame>(case_count);
                break;
            }
            case OP_SELECT_RECV: {
                uint16_t jump_offset = read_short(current_frame);
                uint8_t slot = read_byte(current_frame);

                Value pipe_val = pop();

                // select case is disabled
                if (pipe_val.is_null()) {
                    current_thread->active_select->add_recv_case(nullptr, current_frame.ip + jump_offset - 1, slot);
                } else {
                    if (!pipe_val.is_pipe()) throw std::runtime_error("Expected a pipe for SELECT_RECV");

                    auto pipe = pipe_val.as_pipe();
                    if (!pipe) throw std::runtime_error("Invalid pipe in SELECT_RECV");

                    if (pipe->closed && pipe->buffer.empty() && pipe->writers.empty()) {
                        current_thread->active_select->add_recv_case(nullptr, current_frame.ip + jump_offset - 1, slot);
                    } else {
                        current_thread->active_select->add_recv_case(pipe, current_frame.ip + jump_offset - 1, slot);
                    }
                }

                if (slot != 0xFF) {
                    current_thread->ctx.stack_size = std::max(current_thread->ctx.stack_size, static_cast<size_t>(slot + 1));
                    current_thread->ctx.stack[slot] = {};
                }
                break;
            }
            case OP_SELECT_SEND: {
                uint16_t jump_offset = read_short(current_frame);

                Value val = pop();
                Value pipe_val = pop();

                // select case is disabled
                if (pipe_val.is_null()) {
                    current_thread->active_select->add_send_case(nullptr, current_frame.ip + jump_offset, val);
                    break;
                }

                if (!pipe_val.is_pipe()) throw std::runtime_error("Expected a pipe for SELECT_SEND");

                auto pipe = pipe_val.as_pipe();
                if (!pipe) throw std::runtime_error("Invalid pipe in SELECT_SEND");

                current_thread->active_select->add_send_case(pipe, current_frame.ip + jump_offset, val);
                break;
            }
            case OP_SELECT_DEFAULT: {
                uint16_t jump_offset = read_short(current_frame);
                current_thread->active_select->add_default_case(current_frame.ip + jump_offset);
                break;
            }
            case OP_SELECT_EXEC: {
                if (current_thread->active_select->execute(*this, current_frame.ip)) {
                    current_thread->active_select.reset();
                }
                break;
            }
            case OP_NOT:
            case OP_NEG:
            case OP_BIT_NOT: {
                unary_op(op);
                break;
            }
            case OP_JUMP: {
                int off = static_cast<int16_t>(read_short(current_frame));
                current_frame.ip += off;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                int off = static_cast<int16_t>(read_short(current_frame));
                Value cond = peek(0);
                if (!cond.is_truthy()) {
                    current_frame.ip += off;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                int off = static_cast<int16_t>(read_short(current_frame));
                Value cond = peek(0);
                if (cond.is_truthy()) {
                    current_frame.ip += off;
                }
                break;
            }
            case OP_CALL: {
                uint8_t arg_count = read_byte(current_frame);
                Value callee = peek(arg_count);
                call_value(callee, arg_count);
                break;
            }
            case OP_MAKE_ARRAY: {
                uint16_t count = read_short(current_frame);
                std::vector<Value> elements;
                for (uint16_t i = 0; i < count; ++i) {
                    elements.push_back(pop());
                }
                std::reverse(elements.begin(), elements.end());

                push(heap->allocate<Array>(elements));
                break;
            }
            case OP_MAKE_OBJECT: {
                uint16_t count = read_short(current_frame);
                std::unordered_map<std::string, Value> map;
                for (uint16_t i = 0; i < count; ++i) {
                    Value key = pop();
                    Value val = pop();
                    if (!key.is_string()) throw std::runtime_error("Record keys must be strings");
                    map[key.as_string()] = val;
                }

                push(heap->allocate<Record>(map));
                break;
            }
            case OP_STRUCT: {
                uint16_t name_idx = read_short(current_frame);
                auto name_val = chunk.constants[name_idx];
                if (!name_val.is_string()) {
                    throw std::runtime_error("Expected string for STRUCT name");
                }

                std::string struct_name = name_val.as_string();
                // auto strct = std::make_shared<Struct>(struct_name);
                auto strct = heap->allocate<Struct>(struct_name);
                push(strct);
                break;
            }
            case OP_METHOD: {
                uint16_t name_idx = read_short(current_frame);
                auto name_val = chunk.constants[name_idx];
                if (!name_val.is_string()) {
                    throw std::runtime_error("Expected string for METHOD name");
                }

                std::string method_name = name_val.as_string();
                Value method_func = pop();
                Value struct_val = peek(0);
                if (!struct_val.is_struct()) {
                    throw std::runtime_error("METHOD must be defined on a STRUCT");
                }

                auto strct = struct_val.as_struct();
                strct->add_method(method_name, method_func);
                break;
            }
            case OP_SPAWN: {
                auto thread_count_val = pop();
                if (!thread_count_val.is_int()) {
                    throw std::runtime_error("Expected integer for SPAWN thread count");
                }

                size_t thread_count = static_cast<size_t>(thread_count_val.as_int());
                // Value closure_val = pop();
                Value closure_val = peek();
                if (!closure_val.is_closure()) {
                    throw std::runtime_error("Expected closure for SPAWN");
                }

                auto closure = closure_val.as_closure();
                spawn_thread(closure, thread_count);

                Value thread_handle = pop();
                poke(thread_handle); // replace closure on stack with thread handle

                break;
            }
            default:
                throw std::runtime_error("Unknown opcode " + std::to_string((int)op));
        }

        if (current_thread->state != GreenThread::Running) [[unlikely]] return;
    }
}

void VM::dump_profile() {
    std::vector<std::pair<uint8_t, uint64_t>> entries;
    entries.reserve(256);

    for (size_t i = 0; i < opcode_counts.size(); ++i) {
        if (opcode_counts[i] > 0) {
            entries.emplace_back(static_cast<uint8_t>(i), opcode_counts[i]);
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    std::cerr << "\n=== VM Opcode Profile ===\n";
    std::cerr << "Total instructions: " << total_instructions << "\n";

    double total_ms = total_time_ns / 1e6;
    std::cerr << std::fixed << std::setprecision(3);
    std::cerr << "Total time: " << total_ms << " ms\n";

    std::cerr << "Top opcodes:\n";

    const size_t max_rows = 20;
    for (size_t i = 0; i < entries.size() && i < max_rows; ++i) {
        auto op = static_cast<OpCode>(entries[i].first);
        double pct = total_instructions > 0
            ? (100.0 * static_cast<double>(entries[i].second) / static_cast<double>(total_instructions))
            : 0.0;
        std::cerr << "  " << opcode_to_string(op) << ": " << entries[i].second
                  << " (" << pct << "%)";

        uint64_t ns = opcode_time_ns[entries[i].first];
        double ms = ns / 1e6;
        double tpct = total_time_ns > 0
            ? (100.0 * static_cast<double>(ns) / static_cast<double>(total_time_ns))
            : 0.0;
        double avg_ns = entries[i].second > 0
            ? static_cast<double>(ns) / static_cast<double>(entries[i].second)
            : 0.0;
        std::cerr << ", " << ms << " ms (" << tpct << "%)"
                  << ", avg " << avg_ns << " ns\n";
    }
    std::cerr << "=========================\n";
}

inline void VM::unary_op(OpCode op) {
    Value v = pop();

    switch (op) {
        case OP_NOT:     push(!v); break;
        case OP_NEG:     push(-v); break;
        case OP_BIT_NOT: push(~v); break;
        default:
            throw std::runtime_error("Unknown unary opcode");
    }
}

Array* VM::concat_array(Array* a, Array* b) {
    std::vector<Value> combined(a->size() + b->size());
    combined.insert(combined.end(), a->begin(), a->end());
    combined.insert(combined.end(), b->begin(), b->end());
    return heap->allocate<Array>(combined);
}

ByteArray* VM::concat_bytearray(ByteArray* a, ByteArray* b) {
    std::vector<uint8_t> combined(a->size() + b->size());
    combined.insert(combined.end(), a->begin(), a->end());
    combined.insert(combined.end(), b->begin(), b->end());
    return heap->allocate<ByteArray>(combined);
}

void VM::binary_op(OpCode op) {
    Value b = pop();
    Value a = pop();

    switch (op) {
        case OP_ADD: {
            if (a.is_array() && b.is_array()) {
                push(concat_array(a.as_array(), b.as_array()));
            } else if (a.is_byte_array() && b.is_byte_array()) {
                push(concat_bytearray(a.as_byte_array(), b.as_byte_array()));
            } else {
                push(a + b);
            }
            break;
        }
        case OP_SUB: push(a - b);  break;
        case OP_MUL: {
            if ((a.is_array() && b.is_int()) || (a.is_int() && b.is_array())) {
                Array* arr;
                int times;

                if (a.is_array() && b.is_int()) {
                    arr = a.as_array();
                    times = b.as_int();
                } else {
                    arr = b.as_array();
                    times = a.as_int();
                }

                if (times < 0) {
                    throw std::runtime_error("Cannot multiply array by negative integer");
                }

                std::vector<Value> combined;
                combined.reserve(arr->elements.size() * static_cast<size_t>(times));
                for (int i = 0; i < times; ++i) {
                    combined.insert(combined.end(), arr->elements.begin(), arr->elements.end());
                }
                push(heap->allocate<Array>(combined));
            } else {
                push(a * b);
            }
            break;
        }
        case OP_DIV:         push(a / b);  break;
        case OP_MOD:         push(a % b);  break;
        case OP_EQ:          push(a == b); break;
        case OP_NEQ:         push(a != b); break;
        case OP_LT:          push(a < b);  break;
        case OP_LE:          push(a <= b); break;
        case OP_GE:          push(a >= b); break;
        case OP_GT:          push(a > b);  break;
        case OP_BIT_AND:     push(a & b);  break;
        case OP_BIT_OR:      push(a | b);  break;
        case OP_BIT_XOR:     push(a ^ b);  break;
        case OP_SHIFT_LEFT:  push(a << b); break;
        case OP_SHIFT_RIGHT: push(a >> b); break;
        default: break;
    }
}
