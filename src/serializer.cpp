#include "green_thread.hpp"
#include "value.hpp"
#include "serializer.hpp"
#include "runtime.hpp"
#include "vm.hpp"

void JsonSerializer::print_vm(const VM &vm) {
    // print all visited objects
    out << "{\n";

    {
        IndentGuard guard(indent_level);
        out << indent() << "\"objects\": {\n";

        {
            IndentGuard guard(indent_level);
            for (size_t i = 0; i < vm.heap->objects.size(); i++) {
                Object* obj = vm.heap->objects[i];
                out << indent() << "\"" << obj->id << "\": {\n";
                {
                    IndentGuard guard(indent_level);
                    obj->serialize(*this);
                }
                out << indent() << "}";
                if (i < vm.heap->objects.size() - 1) out << ",";
                out << "\n";
            }
        }

        out << indent() << "},\n";
    }

    {
        IndentGuard guard(indent_level);
        out << indent() << "\"globals\": {\n";

        {
            IndentGuard guard(indent_level);
            size_t count = 0;
            for (const auto &pair : vm.globals) {
                out << indent() << "\"" << pair.first << "\": ";
                print_value(pair.second);
                if (count < vm.globals.size() - 1) out << ",";
                out << "\n";
                count++;
            }
        }

        out << indent() << "},\n";
    }

    {
        IndentGuard guard(indent_level);
        out << indent() << "\"main_thread_id\": " << (vm.main_thread ? std::to_string(vm.main_thread->id) : "null") << ",\n";
        out << indent() << "\"current_thread_id\": " << (vm.current_thread ? std::to_string(vm.current_thread->id) : "null") << ",\n";
        out << indent() << "\"cli_arguments\": ";
        print_value(vm.cli_arguments);
        out << ",\n";
        out << indent() << "\"scheduler\": {\n";
        {
            IndentGuard guard(indent_level);
            out << indent() << "\"next_thread_id\": " << vm.scheduler.next_thread_id << ",\n";
            out << indent() << "\"next_pipe_id\": " << vm.scheduler.next_pipe_id << ",\n";
            out << indent() << "\"ready_queue\": [";
            for (size_t i = 0; i < vm.scheduler.ready_queue.size(); i++) {
                out << vm.scheduler.ready_queue[i]->id;
                if (i < vm.scheduler.ready_queue.size() - 1) out << ", ";
            }
            out << "],\n";

            auto blocked = vm.scheduler.blocked_queue;
            out << indent() << "\"blocked_queue\": [\n";
            {
                IndentGuard guard(indent_level);
                size_t blocked_count = blocked.size();
                for (size_t i = 0; i < blocked_count; i++) {
                    const auto entry = blocked.top();
                    blocked.pop();
                    out << indent() << "{\n";
                    {
                        IndentGuard guard(indent_level);
                        out << indent() << "\"thread_id\": " << entry.second->id << ",\n";
                        auto rem = entry.first - now;
                        if (rem.count() <= 0) {
                            out << indent() << "\"wake_time_rem\": 0\n";
                        } else {
                            out << indent() << "\"wake_time_rem\": " << std::chrono::duration_cast<std::chrono::milliseconds>(rem).count() << "\n";
                        }
                    }
                    out << indent() << "}";
                    if (i < blocked_count - 1) out << ",";
                    out << "\n";
                }
            }
            out << indent() << "]\n";
        }
        out << indent() << "}\n";
    }

    out << indent() << "}\n";
}

void BinarySerializer::print_vm(const VM& vm) {
    // print all visited objects

    const char magic_word[] = "dog:vm-state";

    print_buffer(magic_word, sizeof(magic_word));

    if (vm.heap->next_id - 1 <= UINT8_MAX) {
        id_size = 1;
    } else if (vm.heap->next_id - 1 <= UINT16_MAX) {
        id_size = 2;
    } else if (vm.heap->next_id - 1 <= UINT32_MAX) {
        id_size = 4;
    } else {
        id_size = 8;
    }

    print_number<size_t>(vm.heap->objects.size());

    for (auto obj : vm.heap->objects) {
        print_id(obj->id);
        obj->serialize(*this);
    }

    print_number<size_t>(vm.globals.size());

    for (const auto &[key, val] : vm.globals) {
        print_string(key);
        print_value(val);
    }

    print_number<bool>(vm.main_thread != nullptr);
    if (vm.main_thread) {
        print_id(vm.main_thread->id);
    }

    print_number<bool>(vm.current_thread != nullptr);
    if (vm.current_thread) {
        print_id(vm.current_thread->id);
    }

    print_value(vm.cli_arguments);

    print_number<size_t>(vm.scheduler.next_thread_id);
    print_number<size_t>(vm.scheduler.next_pipe_id);

    print_number<size_t>(vm.scheduler.ready_queue.size());
    for (size_t i = 0; i < vm.scheduler.ready_queue.size(); i++) {
        print_id(vm.scheduler.ready_queue[i]->id);
    }

    auto blocked = vm.scheduler.blocked_queue;
    print_number<size_t>(blocked.size());
    while (!blocked.empty()) {
        const auto entry = blocked.top();
        blocked.pop();
        print_id(entry.second->id);
        auto rem = entry.first - now;
        if (rem.count() <= 0) {
            print_number<size_t>(0);
        } else {
            print_number<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(rem).count());
        }
    }
}

void JsonSerializer::print_value(const Value &val) {
    if (val.is_object()) {
        out << "{\n";
        {
            IndentGuard guard(indent_level);
            out << indent() << "\"object_id\": " << val.as_object()->id << ",\n";
            out << indent() << "\"type\": \"" << val.as_object()->type_name() << "\"\n";
        }
        out << indent() << "}";
    } else if (val.is_string()) {
        out << "\"" << val.as_string() << "\"";
    } else {
        out << val.to_string();
    }
}

// n = null
// i = int
// f = float
// b = bool
// s = string
// o = object
//
// F = function
// N = native function
// C = closure
// M = method closure
// U = upvalue
// A = array
// R = record
// B = byte array
// S = struct
// I = struct instance
// T = green thread
// P = pipe
// H = i/o handle
//
void BinarySerializer::print_value(const Value &val) {
    if (val.is_null()) {
        out.write("n", 1);
    } else if (val.is_int()) {
        out.write("i", 1);
        print_number<int>(val.as_int());
    } else if (val.is_float()) {
        out.write("f", 1);
        print_number<double>(val.as_float());
    } else if (val.is_bool()) {
        out.write("b", 1);
        print_number<bool>(val.as_bool());
    } else if (val.is_string()) {
        out.write("s", 1);
        print_string(val.as_string());
    } else if (val.is_object()) {
        out.write("o", 1);
        print_id(val.as_object()->id);
    }
}

void JsonSerializer::print_function(Function* func) {
    out << indent() << "\"type\": \"Function\",\n";
    out << indent() << "\"name\": \"" << func->name << "\",\n";
    out << indent() << "\"arity\": " << func->arity << ",\n";
    out << indent() << "\"upvalue_count\": " << func->upvalue_count << ",\n";
    out << indent() << "\"chunk\": {\n";
    {
        IndentGuard guard(indent_level);
        out << indent() << "\"code\": [";
        for (size_t i = 0; i < func->chunk.code.size(); i++) {
            out << static_cast<int>(func->chunk.code[i]);
            if (i < func->chunk.code.size() - 1) out << ", ";
        }
        out << "],\n";

        out << indent() << "\"constants\": [\n";
        {
            IndentGuard guard(indent_level);
            for (size_t i = 0; i < func->chunk.constants.size(); i++) {
                out << indent();
                print_value(func->chunk.constants[i]);
                if (i < func->chunk.constants.size() - 1) out << ",";
                out << "\n";
            }
        }
        out << indent() << "]\n";
    }
    out << indent() << "}\n";
}

void BinarySerializer::print_function(Function* func) {
    out.write("F", 1);

    print_string(func->name);
    print_number<size_t>(func->arity);
    print_number<size_t>(func->upvalue_count);

    print_number<size_t>(func->chunk.code.size());

    for (size_t i = 0; i < func->chunk.code.size(); i++) {
        print_number<uint8_t>(func->chunk.code[i]);
    }

    print_number<size_t>(func->chunk.constants.size());

    for (size_t i = 0; i < func->chunk.constants.size(); i++) {
        print_value(func->chunk.constants[i]);
    }
}

void JsonSerializer::print_native(Native* native) {
    out << indent() << "\"type\": \"Native\",\n";
    out << indent() << "\"name\": \"" << native->name << "\",\n";
    out << indent() << "\"arity\": " << native->arity << ",\n";
    out << indent() << "\"bound_instance\": ";
    print_value(native->bound_instance);
    out << "\n";
}

void BinarySerializer::print_native(Native* native) {
    out.write("N", 1);

    print_string(native->name);
    print_number<size_t>(native->arity);
    print_value(native->bound_instance);
}

void JsonSerializer::print_closure(Closure* closure) {
    out << indent() << "\"type\": \"Closure\",\n";
    out << indent() << "\"func_id\": " << closure->func->id << ",\n";
    out << indent() << "\"upvalues\": [\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < closure->upvalues.size(); i++) {
            Upvalue* upvalue = closure->upvalues[i];
            out << indent();
            print_value(upvalue);
            if (i < closure->upvalues.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "]\n";
}

void BinarySerializer::print_closure(Closure* closure) {
    out.write("C", 1);

    print_id(closure->func->id);
    print_number<size_t>(closure->upvalues.size());

    for (size_t i = 0; i < closure->upvalues.size(); i++) {
        print_value(closure->upvalues[i]);
    }
}

void JsonSerializer::print_method_closure(MethodClosure* method_closure) {
    out << indent() << "\"type\": \"MethodClosure\",\n";
    out << indent() << "\"closure_id\": " << method_closure->closure->id << ",\n";
    out << indent() << "\"self\": ";
    print_value(method_closure->self);
    out << "\n";
}

void BinarySerializer::print_method_closure(MethodClosure* method_closure) {
    out.write("M", 1);

    print_id(method_closure->closure->id);
    print_value(method_closure->self);
}

void JsonSerializer::print_upvalue(Upvalue* upvalue) {
    out << indent() << "\"type\": \"Upvalue\",\n";
    out << indent() << "\"owner_thread_id\": " << (upvalue->owner_thread ? std::to_string(upvalue->owner_thread->id) : "null") << ",\n";
    out << indent() << "\"slot_index\": " << upvalue->slot_index << ",\n";
    out << indent() << "\"closed\": ";
    print_value(upvalue->closed);
    out << "\n";
}

void BinarySerializer::print_upvalue(Upvalue* upvalue) {
    out.write("U", 1);

    print_id(upvalue->owner_thread ? -1 : upvalue->owner_thread->id);
    print_number<int>(upvalue->slot_index);
    print_value(upvalue->closed);
}

void JsonSerializer::print_array(Array* array) {
    out << indent() << "\"type\": \"Array\",\n";
    out << indent() << "\"elements\": [\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < array->elements.size(); i++) {
            out << indent();
            print_value(array->elements[i]);
            if (i < array->elements.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "]\n";
}

void BinarySerializer::print_array(Array* array) {
    out.write("A", 1);

    print_number<size_t>(array->size());

    for (size_t i = 0; i < array->size(); i++) {
        print_value((*array)[i]);
    }
}

void JsonSerializer::print_record(Record* record) {
    out << indent() << "\"type\": \"Record\",\n";
    out << indent() << "\"fields\": {\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < record->items.size(); i++) {
            const auto &[key, value] = *std::next(record->items.begin(), i);
            out << indent() << "\"" << key << "\": ";
            print_value(value);
            if (i < record->items.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "}\n";
}

void BinarySerializer::print_record(Record* record) {
    out.write("R", 1);

    print_number<size_t>(record->size());

    for (size_t i = 0; i < record->size(); i++) {
        const auto &[key, value] = *std::next(record->begin(), i);
        print_string(key);
        print_value(value);
    }
}

void JsonSerializer::print_byte_array(ByteArray* byte_array) {
    out << indent() << "\"type\": \"ByteArray\",\n";
    out << indent() << "\"data\": [";
    for (size_t i = 0; i < byte_array->data.size(); i++) {
        out << static_cast<int>(byte_array->data[i]);
        if (i < byte_array->data.size() - 1) out << ", ";
    }
    out << "]\n";
}

void BinarySerializer::print_byte_array(ByteArray* byte_array) {
    out.write("B", 1);

    print_number<size_t>(byte_array->size());
    print_buffer(reinterpret_cast<const char*>(byte_array->data.data()), byte_array->size());
}

void JsonSerializer::print_struct(Struct* strct) {
    out << indent() << "\"type\": \"Struct\",\n";
    out << indent() << "\"name\": \"" << strct->name << "\",\n";
    out << indent() << "\"methods\": {\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < strct->methods.size(); i++) {
            const auto &[name, method] = *std::next(strct->methods.begin(), i);
            out << indent() << "\"" << name << "\": ";
            print_value(method);
            if (i < strct->methods.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "}\n";
}

void BinarySerializer::print_struct(Struct* strct) {
    out.write("S", 1);

    print_string(strct->name);

    print_number<size_t>(strct->methods.size());

    for (size_t i = 0; i < strct->methods.size(); i++) {
        const auto &[key, value] = *std::next(strct->methods.begin(), i);
        print_string(key);
        print_value(value);
    }
}

void JsonSerializer::print_struct_instance(StructInstance* instance) {
    out << indent() << "\"type\": \"StructInstance\",\n";
    out << indent() << "\"struct_id\": " << instance->struct_ptr->id << ",\n";
    out << indent() << "\"fields\": {\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < instance->fields.size(); i++) {
            const auto &[name, field] = *std::next(instance->fields.begin(), i);
            out << indent() << "\"" << name << "\": ";
            print_value(field);
            if (i < instance->fields.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "}\n";
}

void BinarySerializer::print_struct_instance(StructInstance* instance) {
    out.write("I", 1);

    print_id(instance->id);

    print_number<size_t>(instance->fields.size());

    for (size_t i = 0; i < instance->fields.size(); i++) {
        const auto &[key, value] = *std::next(instance->fields.begin(), i);
        print_string(key);
        print_value(value);
    }
}

void JsonSerializer::print_thread(GreenThread* thread) {
    out << indent() << "\"type\": \"Thread\",\n";
    out << indent() << "\"ID\": " << thread->ID << ",\n";
    out << indent() << "\"state\": " << static_cast<int>(thread->state) << ",\n";
    auto rem = thread->wake_time - now;
    if (rem.count() <= 0) {
        out << indent() << "\"wake_time_rem\": 0,\n";
    } else {
        out << indent() << "\"wake_time_rem\": " << std::chrono::duration_cast<std::chrono::milliseconds>(rem).count() << ",\n";
    }
    out << indent() << "\"detached\": " << (thread->detached ? "true" : "false") << ",\n";
    out << indent() << "\"return_value\": ";
    print_value(thread->return_value);
    out << ",\n";
    out << indent() << "\"pending_value\": ";
    print_value(thread->pending_value);
    out << ",\n";
    out << indent() << "\"parent_id\": " << (thread->parent ? std::to_string(thread->parent->id) : "null") << ",\n";
    out << indent() << "\"joiner_ids\": [";
    for (size_t i = 0; i < thread->joiners.size(); i++) {
        out << thread->joiners[i]->id;
        if (i < thread->joiners.size() - 1) out << ", ";
    }
    out << "],\n";
    out << indent() << "\"child_ids\": [";
    for (size_t i = 0; i < thread->children.size(); i++) {
        out << thread->children[i]->id;
        if (i < thread->children.size() - 1) out << ", ";
    }
    out << "],\n";
    out << indent() << "\"active_select\": ";
    if (!thread->active_select) {
        out << "null,\n";
    } else {
        out << "{\n";
        {
            IndentGuard guard(indent_level);
            out << indent() << "\"cases\": [\n";
            {
                IndentGuard guard(indent_level);
                for (size_t i = 0; i < thread->active_select->cases.size(); i++) {
                    const auto &sel_case = thread->active_select->cases[i];
                    out << indent() << "{\n";
                    {
                        IndentGuard guard(indent_level);
                        out << indent() << "\"type\": " << static_cast<int>(sel_case.type) << ",\n";
                        out << indent() << "\"pipe_id\": " << (sel_case.pipe ? std::to_string(sel_case.pipe->id) : "null") << ",\n";
                        out << indent() << "\"target_ip\": " << sel_case.target_ip << ",\n";
                        out << indent() << "\"slot\": ";
                        if (sel_case.type == SelectCase::Recv) {
                            out << static_cast<int>(sel_case.slot);
                        } else {
                            out << "null";
                        }
                        out << ",\n";
                        out << indent() << "\"value\": ";
                        print_value(sel_case.value);
                        out << "\n";
                    }
                    out << indent() << "}";
                    if (i < thread->active_select->cases.size() - 1) out << ",";
                    out << "\n";
                }
            }
            out << indent() << "],\n";
            out << indent() << "\"has_default\": " << (thread->active_select->has_default ? "true" : "false") << ",\n";
            out << indent() << "\"default_target_ip\": ";
            if (thread->active_select->has_default) {
                out << thread->active_select->default_target_ip;
            } else {
                out << "null";
            }
            out << "\n";
        }
        out << indent() << "},\n";
    }
    out << indent() << "\"ctx\": {\n";
    {
        IndentGuard guard(indent_level);
        out << indent() << "\"stack\": [\n";
        {
            IndentGuard guard(indent_level);
            for (size_t i = 0; i < thread->ctx.stack_size; i++) {
                out << indent();
                print_value(thread->ctx.stack[i]);
                if (i < thread->ctx.stack_size - 1) out << ",";
                out << "\n";
            }
        }
        out << indent() << "],\n";
        out << indent() << "\"callframes\": [\n";
        {
            IndentGuard guard(indent_level);
            for (size_t i = 0; i < thread->ctx.frames.size(); i++) {
                const CallFrame& frame = thread->ctx.frames[i];
                out << indent() << "{\n";
                {
                    IndentGuard guard(indent_level);
                    out << indent() << "\"closure_id\": " << frame.closure->id << ",\n";
                    out << indent() << "\"base\": " << frame.base << ",\n";
                    out << indent() << "\"ip\": " << frame.ip << "\n";
                }
                out << "}\n";
            }
        }
        out << indent() << "],\n";
        out << indent() << "\"open_upvalue_ids\": [";
        for (size_t i = 0; i < thread->ctx.open_upvalues.size(); i++) {
            out << thread->ctx.open_upvalues[i]->id;
            if (i < thread->ctx.open_upvalues.size() - 1) out << ",";
        }
        out << "]\n";
    }
    out << indent() << "}\n";
}

void BinarySerializer::print_thread(GreenThread* thread) {
    out.write("T", 1);

    print_id(thread->ID);
    print_number<uint8_t>(thread->state);

    auto rem = thread->wake_time - now;
    if (rem.count() <= 0) {
        print_number<size_t>(0);
    } else {
        print_number<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(rem).count());
    }

    print_number<bool>(thread->detached);
    print_value(thread->return_value);
    print_value(thread->pending_value);

    print_number<bool>(thread->parent != nullptr);
    if (thread->parent) {
        print_id(thread->parent->id);
    }

    print_number<size_t>(thread->joiners.size());
    for (size_t i = 0; i < thread->joiners.size(); i++) {
        print_id(thread->joiners[i]->id);
    }

    print_number<size_t>(thread->children.size());
    for (size_t i = 0; i < thread->children.size(); i++) {
        print_id(thread->children[i]->id);
    }

    print_number<bool>(thread->active_select != nullptr);
    if (thread->active_select) {
        print_number<size_t>(thread->active_select->cases.size());
        for (size_t i = 0; i < thread->active_select->cases.size(); i++) {
            const auto &sel_case = thread->active_select->cases[i];
            print_number<uint8_t>(sel_case.type);
            print_number<bool>(sel_case.pipe != nullptr);
            if (sel_case.pipe) {
                print_id(sel_case.pipe->id);
            }
            print_number<int>(sel_case.target_ip);
            if (sel_case.type == SelectCase::Recv) {
                print_number<uint8_t>(sel_case.slot);
            } else {
                print_value(sel_case.value);
            }
        }

        print_number<bool>(thread->active_select->has_default);
        if (thread->active_select->has_default) {
            print_number<int>(thread->active_select->default_target_ip);
        }
    }


    print_number<size_t>(thread->ctx.stack_size);
    for (size_t i = 0; i < thread->ctx.stack_size; i++) {
        print_value(thread->ctx.stack[i]);
    }

    print_number<size_t>(thread->ctx.frames.size());
    for (size_t i = 0; i < thread->ctx.frames.size(); i++) {
        const auto& frame = thread->ctx.frames[i];
        print_id(frame.closure->id);
        print_number<int>(frame.base);
        print_number<int>(frame.ip);
    }

    print_number<size_t>(thread->ctx.open_upvalues.size());
    for (size_t i = 0; i < thread->ctx.open_upvalues.size(); i++) {
        print_id(thread->ctx.open_upvalues[i]->id);
    }
}

void JsonSerializer::print_io_handle(IOHandle* handle) {
    out << indent() << "\"type\": \"IOHandle\",\n";
    out << indent() << "\"kind\": " << static_cast<int>(handle->kind) << ",\n";
    out << indent() << "\"fd\": " << handle->fd << ",\n";
    out << indent() << "\"closed\": " << (handle->closed ? "true" : "false") << ",\n";
    out << indent() << "\"read_interest\": " << (handle->read_interest ? "true" : "false") << ",\n";
    out << indent() << "\"write_interest\": " << (handle->write_interest ? "true" : "false") << ",\n";
    out << indent() << "\"metadata\": {\n";
    {
        IndentGuard guard(indent_level);
        out << indent() << "\"path\": \"" << handle->metadata.path << "\",\n";
        out << indent() << "\"local_address\": \"" << handle->metadata.local_address << "\",\n";
        out << indent() << "\"remote_address\": \"" << handle->metadata.remote_address << "\"\n";
    }
    out << indent() << "},\n";

    auto print_ops = [&](const char* label, const std::deque<IOOperation> &ops) {
        out << indent() << "\"" << label << "\": [\n";
        {
            IndentGuard guard(indent_level);
            for (size_t i = 0; i < ops.size(); i++) {
                const auto &op = ops[i];
                out << indent() << "{\n";
                {
                    IndentGuard guard(indent_level);
                    out << indent() << "\"type\": " << static_cast<int>(op.type) << ",\n";
                    out << indent() << "\"thread_id\": " << (op.thread ? std::to_string(op.thread->id) : "null") << ",\n";
                    out << indent() << "\"nbytes\": ";
                    if (op.type == IOOperation::Type::ReadNumBytes) {
                        out << op.nbytes;
                    } else {
                        out << "null";
                    }
                    out << ",\n";
                    out << indent() << "\"delimiter\": ";
                    if (op.type == IOOperation::Type::ReadUntilDelimiter) {
                        out << static_cast<int>(op.delimiter);
                    } else {
                        out << "null";
                    }
                    out << ",\n";
                    out << indent() << "\"write_progress\": ";
                    if (op.type == IOOperation::Type::Write) {
                        out << op.write_progress;
                    } else {
                        out << "null";
                    }
                    out << ",\n";
                    out << indent() << "\"write_buffer\": [";
                    for (size_t i = 0; i < op.write_bytes.size(); i++) {
                        out << static_cast<int>(op.write_bytes[i]);
                        if (i < op.write_bytes.size() - 1) out << ", ";
                    }
                    out << "\n";
                }
                out << indent() << "}";
                if (i < ops.size() - 1) out << ",";
                out << "\n";
            }
        }
        out << indent() << "],\n";
    };

    print_ops("reads", handle->reads);
    print_ops("writes", handle->writes);
    print_ops("accepts", handle->accepts);
    print_ops("connects", handle->connects);

    out << indent() << "\"read_buffer\": [";
    for (size_t i = 0; i < handle->read_buffer.size(); i++) {
        out << static_cast<int>(handle->read_buffer[i]);
        if (i < handle->read_buffer.size() - 1) out << ", ";
    }
    out << "]\n";
}

void BinarySerializer::print_io_handle(IOHandle* handle) {
    out.write("H", 1);

    print_number<uint8_t>(static_cast<uint8_t>(handle->kind));
    print_number<int>(handle->fd);
    print_number<bool>(handle->closed);
    print_number<bool>(handle->read_interest);
    print_number<bool>(handle->write_interest);

    print_string(handle->metadata.path);
    print_string(handle->metadata.local_address);
    print_string(handle->metadata.remote_address);

    auto print_ops = [&](const std::deque<IOOperation> &ops) {
        print_number<size_t>(ops.size());
        for (size_t i = 0; i < ops.size(); i++) {
            const auto &op = ops[i];
            print_number<uint8_t>(op.type);
            print_number<bool>(op.thread != nullptr);
            if (op.thread) {
                print_id(op.thread->id);
            }
            switch (op.type) {
                case IOOperation::Type::ReadNumBytes:
                    print_number<size_t>(op.nbytes);
                    break;
                case IOOperation::Type::ReadUntilDelimiter:
                    print_number<uint8_t>(op.delimiter);
                    break;
                case IOOperation::Type::Write:
                    print_number<size_t>(op.write_progress);
                    print_number<size_t>(op.write_bytes.size());
                    if (!op.write_bytes.empty()) {
                        print_buffer(reinterpret_cast<const char*>(op.write_bytes.data()), op.write_bytes.size());
                    }
                    break;
                case IOOperation::Type::ReadAll:
                case IOOperation::Type::Connect:
                case IOOperation::Type::Accept:
                    break;
            }
        }
    };

    print_ops(handle->reads);
    print_ops(handle->writes);
    print_ops(handle->accepts);
    print_ops(handle->connects);

    print_number<size_t>(handle->read_buffer.size());
    if (!handle->read_buffer.empty()) {
        print_buffer(reinterpret_cast<const char*>(handle->read_buffer.data()), handle->read_buffer.size());
    }
}

void JsonSerializer::print_pipe(Pipe* pipe) {
    out << indent() << "\"type\": \"Pipe\",\n";
    out << indent() << "\"ID\": " << pipe->ID << ",\n";
    out << indent() << "\"capacity\": " << pipe->capacity << ",\n";
    out << indent() << "\"closed\": " << (pipe->closed ? "true" : "false") << ",\n";
    out << indent() << "\"buffer\": [\n";
    {
        IndentGuard guard(indent_level);
        for (size_t i = 0; i < pipe->buffer.size(); i++) {
            out << indent();
            print_value(pipe->buffer[i]);
            if (i < pipe->buffer.size() - 1) out << ",";
            out << "\n";
        }
    }
    out << indent() << "],\n";
    out << indent() << "\"reader_ids\": [";
    for (size_t i = 0; i < pipe->readers.size(); i++) {
        out << pipe->readers[i]->id;
        if (i < pipe->readers.size() - 1) out << ",";
    }
    out << "],\n";
    out << indent() << "\"writer_ids\": [";
    for (size_t i = 0; i < pipe->writers.size(); i++) {
        out << pipe->writers[i]->id;
        if (i < pipe->writers.size() - 1) out << ",";
    }
    out << "],\n";
    out << indent() << "\"selector_ids\": [";
    for (size_t i = 0; i < pipe->selectors.size(); i++) {
        out << pipe->selectors[i]->id;
        if (i < pipe->selectors.size() - 1) out << ",";
    }
    out << "]\n";
}

void BinarySerializer::print_pipe(Pipe* pipe) {
    out.write("P", 1);
    print_id(pipe->ID);
    print_number<size_t>(pipe->capacity);
    print_number<bool>(pipe->closed);

    print_number<size_t>(pipe->buffer.size());
    for (size_t i = 0; i < pipe->buffer.size(); i++) {
        print_value(pipe->buffer[i]);
    }

    print_number<size_t>(pipe->readers.size());
    for (size_t i = 0; i < pipe->readers.size(); i++) {
        print_id(pipe->readers[i]->id);
    }

    print_number<size_t>(pipe->writers.size());
    for (size_t i = 0; i < pipe->writers.size(); i++) {
        print_id(pipe->writers[i]->id);
    }

    print_number<size_t>(pipe->selectors.size());
    for (size_t i = 0; i < pipe->selectors.size(); i++) {
        print_id(pipe->selectors[i]->id);
    }
}
