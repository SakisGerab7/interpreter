#include "vm.hpp"
#include "bytecode.hpp"
#include "native_functions.hpp"

VM::VM() {
    // define native functions here if needed
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

    define_native("arange", 3, native_functions::array::arange);
    define_native("Array.push",    1, native_functions::array::push);
    define_native("Array.pop",     0, native_functions::array::pop);
    define_native("Array.shift",   0, native_functions::array::shift);
    define_native("Array.unshift", 1, native_functions::array::unshift);
    define_native("Array.slice",   2, native_functions::array::slice);
    define_native("Array.sum",     0, native_functions::array::sum);

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
    // define_native("Thread.detach", 0, native_functions::detach);

    define_native("pipe", 1, native_functions::pipe);
}

void VM::spawn_thread(Closure::Ptr closure, size_t thread_count) {
    std::vector<Value> handles;
    for (size_t i = 0; i < thread_count; ++i) {
        auto new_thread = std::make_shared<GreenThread>(scheduler.next_thread_id++);
        new_thread->stack[new_thread->stack_size++] = Value(closure);

        CallFrame frame;
        frame.closure = closure;
        frame.ip = 0;
        frame.base = static_cast<int>(new_thread->stack_size) - 1;
        new_thread->frames.push_back(frame);

        scheduler.add_thread(new_thread);
        scheduler.enqueue(new_thread);

        if (current_thread) {
            current_thread->children.push_back(new_thread);
        }

        handles.push_back(ThreadHandle(new_thread->ID));
    }

    if (current_thread) {
        push((thread_count == 1) ? handles[0] : std::make_shared<Array>(handles));
    }
}

Value VM::interpret(Function::Ptr func) {
    auto closure = std::make_shared<Closure>(func);
    spawn_thread(closure, 1);
    return scheduler.schedule(*this);
}

inline void VM::define_native(const std::string &name, int arity, NativeFn func) {
    globals[name] = std::make_shared<Native>(name, arity, func);
}

void VM::call_value(const Value &callee, int arg_count) {
    if (callee.is_closure()) {
        call(callee.as_closure(), arg_count);
    } else if (callee.is_function()) {
        auto func = callee.as_function();
        auto closure = std::make_shared<Closure>(func);
        call(closure, arg_count);
    } else if (callee.is_native()) {
        call_native(callee.as_native(), arg_count);
    } else if (callee.is_struct()) {
        // Creating a new instance of the struct
        auto strct = callee.as_struct();
        current_thread->stack[current_thread->stack_size - arg_count - 1] = std::make_shared<StructInstance>(strct);

        auto it = strct->methods.find("init");
        if (it != strct->methods.end()) {
            auto init_method = it->second;
            call_value(init_method, arg_count);
        } else if (arg_count != 0) {
            throw std::runtime_error("Struct constructor does not take arguments");
        }
    } else {
        throw std::runtime_error("Can only call functions and closures");
    }
}

void VM::call_native(const Native::Ptr &native, int arg_count) {
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

    Value result = native->func(*this, args);
    for (int i = 0; i < arg_count; i++) {
        pop();
    }

    pop(); // pop the native function itself
    push(result);
}

void VM::call(const Closure::Ptr &closure, int arg_count) {
    if (arg_count != closure->func->arity) {
        throw std::runtime_error("Expected " + std::to_string(closure->func->arity) +
                                 " arguments but got " + std::to_string(arg_count));
    }

    if (current_thread->frames.size() >= 256) {
        throw std::runtime_error("Stack overflow");
    }

    if (!closure->recv_self.is_null()) {
        // If this is a method call, set the first argument to recv_self
        current_thread->stack[current_thread->stack_size - arg_count - 1] = closure->recv_self;
        closure->recv_self = {}; // clear after use
    }

    CallFrame frame;
    frame.closure = closure;
    frame.ip = 0;
    frame.base = static_cast<int>(current_thread->stack_size - arg_count - 1);
    current_thread->frames.push_back(frame);
}

inline Upvalue::Ptr VM::capture_upvalue(Value *local) {
    // Check if we already have an open upvalue pointing to this stack slot.
    for (auto &uv : current_thread->open_upvalues) {
        if (uv->location != nullptr && uv->location == local)
            return uv;
    }

    // Otherwise create a new upvalue for this local
    auto up = std::make_shared<Upvalue>(local);
    current_thread->open_upvalues.push_back(up);
    return up;
}

inline void VM::close_upvalues(int last) {
    for (auto it = current_thread->open_upvalues.begin(); it != current_thread->open_upvalues.end(); ) {
        auto upvalue = *it;
        if (upvalue->location != nullptr && upvalue->location >= &current_thread->stack[last]) {
            // Move the value from the stack to the upvalue's closed field
            upvalue->closed = *(upvalue->location);
            upvalue->location = nullptr; // now points to closed value
            it = current_thread->open_upvalues.erase(it);
        } else {
            ++it;
        }
    }
}

inline uint8_t VM::read_byte(CallFrame &frame) {
    return frame.closure->func->chunk.code[frame.ip++];
}

inline uint16_t VM::read_short(CallFrame &frame) {
    uint16_t high = read_byte(frame);
    uint16_t low = read_byte(frame);
    return (high << 8) | low;
}

inline void VM::push(const Value& v) {
    if (current_thread->stack_size >= current_thread->stack.size()) {
        throw std::runtime_error("Stack overflow");
    }

    current_thread->stack[current_thread->stack_size++] = v;
}

inline Value VM::pop() {
    if (current_thread->stack_size == 0) {
        throw std::runtime_error("Stack underflow");
    }

    return current_thread->stack[--current_thread->stack_size];
}

inline Value& VM::peek(size_t depth) {
    return current_thread->stack[current_thread->stack_size - 1 - depth];
}

void VM::debug_instruction(CallFrame &frame, OpCode op) {
    std::cerr << "[Thread " << current_thread->ID << "] ";
    std::cerr << "[IP " << std::hex << std::right << std::setw(4)  << std::setfill('0') << (frame.ip - 1)
              << "] "   << std::dec << std::left  << std::setw(15) << std::setfill(' ') << opcode_to_string(op)
              << " | ";

    std::cerr << "Stack: [";
    for (size_t i = 0; i < current_thread->stack_size; ++i) {
        std::cerr << current_thread->stack[i].to_string();
        if (i < current_thread->stack_size - 1) std::cerr << ", ";
    }

    std::cerr << "]\n";
}

void VM::run() {
    while (true) {
        if (current_thread->frames.empty()) {
            current_thread->state = GreenThread::Finished;
            return;
        }

        CallFrame &curr = current_thread->frames.back();
        auto &chunk = curr.closure->func->chunk;
        if (curr.ip >= chunk.code.size()) {
            current_thread->frames.pop_back();
            if (current_thread->frames.empty()) {
                current_thread->state = GreenThread::Finished;
                return;
            }
            continue;
        }

        OpCode op = static_cast<OpCode>(read_byte(curr));

        debug_instruction(curr, op);

        switch (op) {
            case OP_NULL:  push({});    break;
            case OP_TRUE:  push(true);  break;
            case OP_FALSE: push(false); break;
            case OP_CONST: {
                int idx = read_short(curr);
                if (idx >= chunk.constants.size()) throw std::runtime_error("constant index out of range");
                push(chunk.constants[idx]);
                break;
            }
            case OP_ICONST8: {
                int val = static_cast<int8_t>(read_byte(curr));
                push(val);
                break;
            }
            case OP_ICONST16: {
                int val = static_cast<int16_t>(read_short(curr));
                push(val);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint16_t idx = read_short(curr);
                auto name = chunk.constants[idx].as_string();
                globals[name] = pop();
                break;
            }
            case OP_LOAD_GLOBAL: {
                uint16_t idx = read_short(curr);
                auto name = chunk.constants[idx].as_string();
                auto it = globals.find(name);
                if (it == globals.end()) throw std::runtime_error("Undefined global variable: " + name);
                push(it->second);
                break;
            }
            case OP_STORE_GLOBAL: {
                uint16_t idx = read_short(curr);
                auto name = chunk.constants[idx].as_string();
                auto it = globals.find(name);
                if (it == globals.end()) throw std::runtime_error("Undefined global variable: " + name);
                it->second = peek(0);
                break;
            }
            case OP_LOAD_LOCAL: {
                uint8_t local_idx = read_byte(curr);
                int stack_idx = curr.base + local_idx;
                if (stack_idx < 0 || stack_idx >= current_thread->stack_size) {
                    throw std::runtime_error("Local variable index out of range");
                }

                push(current_thread->stack[stack_idx]);
                break;
            }
            case OP_STORE_LOCAL: {
                uint8_t local_idx = read_byte(curr);
                int stack_idx = curr.base + local_idx;
                if (stack_idx < 0 || stack_idx >= current_thread->stack_size) {
                    throw std::runtime_error("Local variable index out of range");
                }

                current_thread->stack[stack_idx] = peek(0);
                break;
            }
            case OP_LOAD_UPVALUE: {
                uint8_t upvalue_idx = read_byte(curr);
                if (upvalue_idx >= curr.closure->upvalues.size()) {
                    throw std::runtime_error("Upvalue index out of range");
                }

                auto upvalue = curr.closure->upvalues[upvalue_idx];
                push(upvalue->get());
                break;
            }
            case OP_STORE_UPVALUE: {
                uint8_t upvalue_idx = read_byte(curr);
                if (upvalue_idx >= curr.closure->upvalues.size()) {
                    throw std::runtime_error("Upvalue index out of range");
                }

                auto upvalue = curr.closure->upvalues[upvalue_idx];
                upvalue->set(peek(0));
                break;
            }
            case OP_LOAD_FIELD: {
                int idx = read_short(curr);
                std::string key = chunk.constants[idx].as_string();
                Value obj = pop();

                if (obj.is_string()) {
                    // Bind 'self' to the instance
                    auto method_it = globals.find("String." + key);
                    if (method_it == globals.end()) {
                        throw std::runtime_error("Undefined method '" + key + "' for String");
                    }
                    auto method = method_it->second.as_native();
                    method->bound_instance = obj;
                    push(method);
                } else if (obj.is_array()) {
                    // Bind 'self' to the instance
                    auto method_it = globals.find("Array." + key);
                    if (method_it == globals.end()) {
                        throw std::runtime_error("Undefined method '" + key + "' for Array");
                    }
                    auto method = method_it->second.as_native();
                    method->bound_instance = obj;
                    push(method);
                } else if (obj.is_thread_handle()) {
                    // Bind 'self' to the instance
                    auto method_it = globals.find("Thread." + key);
                    if (method_it == globals.end()) {
                        throw std::runtime_error("Undefined method '" + key + "' for Thread");
                    }
                    auto method = method_it->second.as_native();
                    method->bound_instance = obj;
                    push(method);
                } else {
                    auto field_val = obj.get_index(key);
                    if (obj.is_struct_instance() && field_val.is_closure()) {
                        // Bind 'self' to the instance
                        auto closure = field_val.as_closure();
                        closure->recv_self = obj;
                        field_val = closure;
                    }

                    push(field_val);
                }
                break;
            }
            case OP_STORE_FIELD: {
                int idx = read_short(curr);
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
                push(container.get_index(index));
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
                uint16_t func_idx = read_short(curr);
                if (func_idx >= chunk.constants.size()) {
                    throw std::runtime_error("Function index out of range");
                }

                auto func_val = chunk.constants[func_idx];
                if (!func_val.is_function()) {
                    throw std::runtime_error("Expected function for CLOSURE opcode");
                }

                auto func = func_val.as_function();
                auto closure = std::make_shared<Closure>(func);

                // capture upvalues
                for (int i = 0; i < func->upvalue_count; i++) {
                    uint8_t is_local = read_byte(curr);
                    uint8_t index = read_byte(curr);
                    if (is_local) {
                        closure->upvalues.push_back(capture_upvalue(&current_thread->stack[curr.base + index]));
                    } else {
                        closure->upvalues.push_back(curr.closure->upvalues[index]);
                    }
                }

                push(closure);
                break;
            }
            case OP_RETURN: {
                Value ret_val = pop();
                close_upvalues(curr.base);
                current_thread->frames.pop_back();
                if (current_thread->frames.empty()) {
                    current_thread->state = GreenThread::Finished;
                    scheduler.set_return_value(current_thread, ret_val);
                    return;
                }

                current_thread->stack_size = curr.base;
                push(ret_val);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(static_cast<int>(current_thread->stack_size) - 1);
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
            case OP_SEND_PIPE: {
                Value val = pop();
                Value pipe_val = pop();
                if (!pipe_val.is_pipe_handle()) {
                    throw std::runtime_error("Expected a pipe handle for SEND_PIPE");
                }

                auto pipe_handle = pipe_val.as_pipe_handle();

                auto pipe = scheduler.get_pipe_by_id(pipe_handle.ID);
                if (!pipe) {
                    throw std::runtime_error("Invalid pipe ID in SEND_PIPE");
                }

                scheduler.send_to_pipe(current_thread, pipe, val);
                push(val);
                break;
            }
            case OP_RECV_PIPE: {
                Value pipe_val = pop();
                if (!pipe_val.is_pipe_handle()) {
                    throw std::runtime_error("Expected a pipe handle for RECV_PIPE");
                }

                auto pipe_handle = pipe_val.as_pipe_handle();

                auto pipe = scheduler.get_pipe_by_id(pipe_handle.ID);
                if (!pipe) {
                    throw std::runtime_error("Invalid pipe ID in RECV_PIPE");
                }

                Value received = scheduler.receive_from_pipe(current_thread, pipe);
                push(received);
                break;
            }
            case OP_CLOSE_PIPE: {
                Value pipe_val = pop();
                if (!pipe_val.is_pipe_handle()) {
                    throw std::runtime_error("Expected a pipe handle for CLOSE_PIPE");
                }

                auto pipe_handle = pipe_val.as_pipe_handle();

                auto pipe = scheduler.get_pipe_by_id(pipe_handle.ID);
                if (!pipe) {
                    throw std::runtime_error("Invalid pipe ID in CLOSE_PIPE");
                }

                scheduler.close_pipe(pipe);
                break;
            }
            case OP_SELECT_BEGIN: {
                uint8_t case_count = read_byte(curr);
                scheduler.select_begin(current_thread, case_count);
                break;
            }
            case OP_SELECT_RECV: {
                uint16_t jump_offset = read_short(curr);
                uint8_t slot = read_byte(curr);

                Value pipe_val = pop();

                // select case is disabled
                if (pipe_val.is_null()) {
                    scheduler.select_add_recv_case(current_thread, nullptr, curr.ip + jump_offset - 1, slot);
                } else {
                    if (!pipe_val.is_pipe_handle()) {
                        throw std::runtime_error("Expected a pipe handle for SELECT_RECV");
                    }

                    auto pipe_handle = pipe_val.as_pipe_handle();

                    auto pipe = scheduler.get_pipe_by_id(pipe_handle.ID);
                    if (!pipe) {
                        throw std::runtime_error("Invalid pipe ID in SELECT_RECV");
                    }

                    scheduler.select_add_recv_case(current_thread, pipe, curr.ip + jump_offset - 1, slot);
                }

                if (slot != 0xFF) {
                    std::cerr << "[Thread " << current_thread->ID << "] SELECT_RECV will store received value in stack slot "
                              << static_cast<int>(slot) << " (stack size: " << current_thread->stack_size << ")\n";

                    current_thread->stack_size = std::max(current_thread->stack_size, static_cast<size_t>(slot + 1));
                    current_thread->stack[slot] = {};
                }
                break;
            }
            case OP_SELECT_SEND: {
                uint16_t jump_offset = read_short(curr);

                Value val = pop();
                Value pipe_val = pop();

                // select case is disabled
                if (pipe_val.is_null()) {
                    scheduler.select_add_send_case(current_thread, nullptr, curr.ip + jump_offset, val);
                    break;
                }

                if (!pipe_val.is_pipe_handle()) {
                    throw std::runtime_error("Expected a pipe handle for SELECT_SEND");
                }

                auto pipe_handle = pipe_val.as_pipe_handle();

                auto pipe = scheduler.get_pipe_by_id(pipe_handle.ID);
                if (!pipe) {
                    throw std::runtime_error("Invalid pipe ID in SELECT_SEND");
                }

                scheduler.select_add_send_case(current_thread, pipe, curr.ip + jump_offset, val);
                break;
            }
            case OP_SELECT_DEFAULT: {
                uint16_t jump_offset = read_short(curr);
                scheduler.select_add_default_case(current_thread, curr.ip + jump_offset);
                break;
            }
            case OP_SELECT_EXEC: {
                scheduler.select_execute(current_thread, curr.ip);
                break;
            }
            case OP_NOT:
            case OP_NEG:
            case OP_BIT_NOT: {
                unary_op(op);
                break;
            }
            case OP_JUMP: {
                int off = static_cast<int16_t>(read_short(curr));
                curr.ip += off;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                int off = static_cast<int16_t>(read_short(curr));
                Value cond = peek(0);
                if (!cond.is_truthy()) {
                    curr.ip += off;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                int off = static_cast<int16_t>(read_short(curr));
                Value cond = peek(0);
                if (cond.is_truthy()) {
                    curr.ip += off;
                }
                break;
            }
            case OP_CALL: {
                uint8_t arg_count = read_byte(curr);
                Value callee = peek(arg_count);
                call_value(callee, arg_count);
                break;
            }
            case OP_MAKE_ARRAY: {
                uint16_t count = read_short(curr);
                std::vector<Value> elements;
                for (uint16_t i = 0; i < count; ++i) {
                    elements.push_back(pop());
                }
                std::reverse(elements.begin(), elements.end());
                push(std::make_shared<Array>(elements));
                break;
            }
            case OP_MAKE_OBJECT: {
                uint16_t count = read_short(curr);
                std::unordered_map<std::string, Value> map;
                for (uint16_t i = 0; i < count; ++i) {
                    Value key = pop();
                    Value val = pop();
                    if (!key.is_string()) throw std::runtime_error("Object keys must be strings");
                    map[key.as_string()] = val;
                }
                push(std::make_shared<Object>(map));
                break;
            }
            case OP_STRUCT: {
                uint16_t name_idx = read_short(curr);
                auto name_val = chunk.constants[name_idx];
                if (!name_val.is_string()) {
                    throw std::runtime_error("Expected string for STRUCT name");
                }

                std::string struct_name = name_val.as_string();
                auto strct = std::make_shared<Struct>(struct_name);
                push(strct);
                break;
            }
            case OP_METHOD: {
                uint16_t name_idx = read_short(curr);
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
                Value closure_val = pop();
                if (!closure_val.is_closure()) {
                    throw std::runtime_error("Expected closure for SPAWN");
                }

                auto closure = closure_val.as_closure();
                spawn_thread(closure, thread_count);
                break;
            }
            default:
                throw std::runtime_error("Unknown opcode " + std::to_string((int)op));
        }

        if (current_thread->state != GreenThread::Running) return;
    }
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

inline void VM::binary_op(OpCode op) {
    Value b = pop();
    Value a = pop();

    switch (op) {
        case OP_ADD:         push(a + b);  break;
        case OP_SUB:         push(a - b);  break;
        case OP_MUL:         push(a * b);  break;
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
    }
}