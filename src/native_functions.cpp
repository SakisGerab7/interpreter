#include "vm.hpp"
#include "native_functions.hpp"

namespace native_functions {
    namespace {
        template <typename UInt>
        std::vector<uint8_t> to_little_endian_bytes(UInt value) {
            std::vector<uint8_t> out(sizeof(UInt));
            for (size_t i = 0; i < sizeof(UInt); ++i) {
                out[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
            }
            return out;
        }

        template <typename UInt>
        UInt from_little_endian_bytes(const std::vector<uint8_t> &bytes) {
            UInt value = 0;
            for (size_t i = 0; i < sizeof(UInt); ++i) {
                value |= static_cast<UInt>(bytes[i]) << (8 * i);
            }
            return value;
        }

        template <typename UInt>
        std::vector<uint8_t> to_big_endian_bytes(UInt value) {
            std::vector<uint8_t> out(sizeof(UInt));
            for (size_t i = 0; i < sizeof(UInt); ++i) {
                out[sizeof(UInt) - 1 - i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
            }
            return out;
        }

        template <typename UInt>
        UInt from_big_endian_bytes(const std::vector<uint8_t> &bytes) {
            UInt value = 0;
            for (size_t i = 0; i < sizeof(UInt); ++i) {
                value |= static_cast<UInt>(bytes[sizeof(UInt) - 1 - i]) << (8 * i);
            }
            return value;
        }

        size_t specifier_size(const std::string &specifier) {
            if (specifier == "i8" || specifier == "u8") return 1;
            if (specifier == "i16" || specifier == "u16") return 2;
            if (specifier == "i32" || specifier == "u32" || specifier == "f32") return 4;
            if (specifier == "f64") return 8;
            throw std::runtime_error("Unsupported specifier: " + specifier);
        }
    }

    Value cli_args(VM &vm, const std::vector<Value> &args) {
        return vm.cli_arguments;
    }

    Value clock(VM &, const std::vector<Value> &args) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis) / 1000.0;
    }

    Value len(VM &, const std::vector<Value> &args) {
        if (args[0].is_array())
            return static_cast<int>(args[0].as_array()->size());
        if (args[0].is_record())
            return static_cast<int>(args[0].as_record()->size());
        if (args[0].is_string())
            return static_cast<int>(args[0].as_string().size());

        return {};
    }

    Value str(VM &, const std::vector<Value> &args) {
        return args[0].to_string();
    }

    Value int_fn(VM &, const std::vector<Value> &args) {
        return args[0].as_int();
    }

    Value float_fn(VM &, const std::vector<Value> &args) {
        return args[0].as_float();
    }

    Value type(VM &, const std::vector<Value> &args) {
        const Value &v = args[0];
        if (v.is_null())     return "null";
        if (v.is_int())      return "int";
        if (v.is_float())    return "float";
        if (v.is_bool())     return "bool";
        if (v.is_string())   return "string";
        if (v.is_array())    return "array";
        if (v.is_record())   return "record";
        if (v.is_struct())   return "type";
        if (v.is_struct_instance()) return std::string(v.as_struct_instance()->struct_ptr->name);
        if (v.is_function() || v.is_closure() || v.is_native()) return "function";
        return "unknown";
    }

    namespace math {
        Value pow(VM &, const std::vector<Value> &args) {
            return std::pow(args[0].as_float(), args[1].as_float());
        }

        Value abs(VM &, const std::vector<Value> &args) {
            return std::abs(args[0].as_float());
        }

        Value round(VM &, const std::vector<Value> &args) {
            return std::round(args[0].as_float());
        }

        Value sqrt(VM &, const std::vector<Value> &args) {
            return std::sqrt(args[0].as_float());
        }

        Value sin(VM &, const std::vector<Value> &args) {
            return std::sin(args[0].as_float());
        }

        Value cos(VM &, const std::vector<Value> &args) {
            return std::cos(args[0].as_float());
        }

        Value tan(VM &, const std::vector<Value> &args) {
            return std::tan(args[0].as_float());
        }

        Value floor(VM &, const std::vector<Value> &args) {
            return std::floor(args[0].as_float());
        }

        Value ceil(VM &, const std::vector<Value> &args) {
            return std::ceil(args[0].as_float());
        }

        Value min(VM &, const std::vector<Value> &args) {
            return std::min(args[0].as_float(), args[1].as_float());
        }

        Value max(VM &, const std::vector<Value> &args) {
            return std::max(args[0].as_float(), args[1].as_float());
        }

        Value srand(VM &, const std::vector<Value> &args) {
            std::srand(args[0].as_int());
            return {};
        }

        Value rand(VM &, const std::vector<Value>&) {
            return static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
        }

        Value randint(VM &, const std::vector<Value> &args) {
            int min = args[0].as_int();
            int max = args[1].as_int();
            return std::rand() % (max - min + 1) + min;
        }

        Value asin(VM &, const std::vector<Value> &args) {
            return std::asin(args[0].as_float());
        }

        Value acos(VM &, const std::vector<Value> &args) {
            return std::acos(args[0].as_float());
        }

        Value atan(VM &, const std::vector<Value> &args) {
            return std::atan(args[0].as_float());
        }

        Value log2(VM &, const std::vector<Value> &args) {
            return std::log2(args[0].as_float());
        }

        Value log10(VM &, const std::vector<Value> &args) {
            return std::log10(args[0].as_float());
        }

        Value ln(VM &, const std::vector<Value> &args) {
            return std::log(args[0].as_float());
        }

        Value exp(VM &, const std::vector<Value> &args) {
            return std::exp(args[0].as_float());
        }
    }

    namespace string {
        Value to_upper(VM &, const std::vector<Value> &args) {
            std::string s = args[0].as_string();
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        }

        Value to_lower(VM &, const std::vector<Value> &args) {
            std::string s = args[0].as_string();
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        }

        Value trim(VM &, const std::vector<Value> &args) {
            std::string s = args[0].as_string();
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
            return s;
        }

        Value split(VM &vm, const std::vector<Value> &args) {
            std::string s = args[0].as_string();
            std::string delimiter = args[1].as_string();
            std::vector<Value> tokens;
            size_t pos = 0;
            while ((pos = s.find(delimiter)) != std::string::npos) {
                tokens.push_back(s.substr(0, pos));
                s.erase(0, pos + delimiter.length());
            }

            tokens.push_back(s);
            return vm.heap->allocate<Array>(tokens);
        }

        Value to_byte_array(VM &vm, const std::vector<Value> &args) {
            const std::string &s = args[0].as_string();
            std::vector<uint8_t> bytes(s.begin(), s.end());
            return vm.heap->allocate<ByteArray>(bytes);
        }
    }

    namespace byte_array {
        Value to_string(VM &, const std::vector<Value> &args) {
            const auto &byte_array = args[0].as_byte_array();
            return std::string(byte_array->data.begin(), byte_array->data.end());
        }
    }

    namespace array {
        static Value invoke_unary_callback(VM &vm, const Value &callback, const Value &arg) {
            if (callback.is_native()) {
                auto native = callback.as_native();
                if (native->arity != 1) {
                    throw std::runtime_error("Callback must take exactly 1 argument");
                }

                std::vector<Value> call_args;
                if (!native->bound_instance.is_null()) {
                    call_args.push_back(native->bound_instance);
                    native->bound_instance = {};
                }

                call_args.push_back(arg);
                return native->func(vm, call_args);
            }

            Value closure_value = callback;
            if (callback.is_function()) {
                closure_value = vm.heap->allocate<Closure>(callback.as_function());
            }

            if (!closure_value.is_closure()) {
                throw std::runtime_error("Callback must be callable");
            }

            auto closure = closure_value.as_closure();

            auto parent_thread = vm.current_thread;
            auto callback_thread = vm.heap->allocate<GreenThread>(vm.scheduler.next_thread_id++);
            callback_thread->state = GreenThread::Running;
            vm.push(closure);
            vm.push(arg);

            vm.current_thread = callback_thread;
            vm.call(closure, 1);

            try {
                vm.run();
            } catch (...) {
                vm.current_thread = parent_thread;
                throw;
            }

            vm.current_thread = parent_thread;
            return callback_thread->return_value;
        }

        Value arange(VM &vm, const std::vector<Value> &args) {
            int start = args[0].as_int();
            int end = args[1].as_int();
            int step = args[2].as_int();
            if (step == 0) {
                throw std::runtime_error("Step cannot be zero");
            }

            std::vector<Value> result;
            if ((step > 0 && start >= end) || (step < 0 && start <= end)) {
                return vm.heap->allocate<Array>(result);
            }

            for (int i = start; (step > 0 ? i < end : i > end); i += step) {
                result.push_back(i);
            }

            return vm.heap->allocate<Array>(result);
        }

        Value push(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            arr->elements.push_back(args[1]);
            return {};
        }

        Value pop(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            if (arr->elements.empty()) throw std::runtime_error("Cannot pop from an empty array");
            Value val = arr->elements.back();
            arr->elements.pop_back();
            return val;
        }

        Value shift(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            if (arr->elements.empty()) throw std::runtime_error("Cannot shift from an empty array");
            Value val = arr->elements.front();
            arr->elements.erase(arr->elements.begin());
            return val;
        }

        Value unshift(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            arr->elements.insert(arr->elements.begin(), args[1]);
            return {};
        }

        Value slice(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            int start = args[1].as_int();
            int end = args[2].as_int();

            if (start < 0 || end > static_cast<int>(arr->elements.size()) || start > end) {
                throw std::runtime_error("Invalid slice indices");
            }

            std::vector<Value> sliced_elements(arr->elements.begin() + start, arr->elements.begin() + end);
            return vm.heap->allocate<Array>(sliced_elements);
        }

        Value sum(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            Value total = 0.0;

            for (const auto &elem : arr->elements) {
                total = total + elem;
            }

            return total;
        }

        Value foreach(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            auto callback = args[1];

            for (const auto &elem : arr->elements) {
                invoke_unary_callback(vm, callback, elem);
            }

            return {};
        }

        Value map(VM &vm, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            auto callback = args[1];

            std::vector<Value> mapped_elements;
            for (const auto &elem : arr->elements) {
                mapped_elements.push_back(invoke_unary_callback(vm, callback, elem));
            }

            return vm.heap->allocate<Array>(mapped_elements);
        }
    }

    Value sleep(VM &vm, const std::vector<Value> &args) {
        int ms = args[0].as_int();
        vm.scheduler.send_to_sleep(vm.current_thread, ms);
        return {};
    }

    Value thread_id(VM &vm, const std::vector<Value> &args) {
        return static_cast<int>(vm.current_thread->ID);
    }

    Value join(VM &vm, const std::vector<Value> &args) {
        auto thread = args[0].as_thread();

        if (vm.current_thread->ID == thread->ID) {
            throw std::runtime_error("Thread cannot join itself");
        }

        if (thread->detached) {
            throw std::runtime_error("Cannot join a detached thread");
        }

        if (!thread || thread->state == GreenThread::Finished) {
            return thread->return_value;
        }

        thread->joiners.push_back(vm.current_thread);
        vm.current_thread->state = GreenThread::Blocked;
        vm.current_thread->wake_time = {};

        std::cerr << "Thread " << vm.current_thread->ID << " is joining on thread " << thread->ID << std::endl;

        return {};
    }

    Value detach(VM &vm, const std::vector<Value> &args) {
        auto thread = args[0].as_thread();

        if (thread->ID == vm.current_thread->ID) {
            throw std::runtime_error("Thread cannot detach itself");
        }

        if (!thread || thread->state == GreenThread::Finished) {
            throw std::runtime_error("Cannot detach a finished thread");
        }

        if (thread->detached) {
            throw std::runtime_error("Thread is already detached");
        }

        if (thread->joiners.size() > 0) {
            throw std::runtime_error("Cannot detach a thread that has joiners");
        }

        thread->detached = true;

        // Remove from parent's children list to allow it to be collected when finished
        if (thread->parent) {
            auto &siblings = thread->parent->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), thread), siblings.end());
            thread->parent = nullptr;
        }

        return {};
    }

    Value pipe(VM &vm, const std::vector<Value> &args) {
        int capacity = args[0].as_int();
        size_t pipe_id = vm.scheduler.next_pipe_id++;
        auto pipe = vm.heap->allocate<Pipe>(pipe_id, capacity);
        return pipe;
    }

    Value listen(VM &vm, const std::vector<Value> &args) {
        std::string type = args[0].as_string();

        if (type == "unix") {
            auto options = args[1].as_record();
            if (auto it = options->find("path"); it != options->end()) {
                std::string path = it->second.as_string();
                return vm.scheduler.socket_unix_listener(path);
            } else {
                throw std::runtime_error("Missing 'path' option for UNIX listener");
            }
        } else if (type == "tcp") {
            auto options = args[1].as_record();
            std::string host;
            uint16_t port;

            if (auto it = options->find("host"); it != options->end()) {
                host = it->second.as_string();
            } else {
                host = "*";
            }

            if (auto it = options->find("port"); it != options->end()) {
                port = static_cast<uint16_t>(it->second.as_int());
            } else {
                throw std::runtime_error("Missing 'port' option for TCP listener");
            }

            return vm.scheduler.socket_tcp_listener(host, port);
        } else {
            throw std::runtime_error("Unsupported socket type: " + type);
        }
    }

    Value connect(VM &vm, const std::vector<Value> &args) {
        std::string type = args[0].as_string();

        if (type == "unix") {
            auto options = args[1].as_record();
            if (auto it = options->find("path"); it != options->end()) {
                std::string path = it->second.as_string();
                return vm.scheduler.socket_unix_connection(path);
            } else {
                throw std::runtime_error("Missing 'path' option for UNIX connection");
            }
        } else if (type == "tcp") {
            auto options = args[1].as_record();
            std::string host;
            uint16_t port;

            if (auto it = options->find("host"); it != options->end()) {
                host = it->second.as_string();
            } else {
                throw std::runtime_error("Missing 'host' option for TCP connection");
            }

            if (auto it = options->find("port"); it != options->end()) {
                port = static_cast<uint16_t>(it->second.as_int());
            } else {
                throw std::runtime_error("Missing 'port' option for TCP connection");
            }

            return vm.scheduler.socket_tcp_connection(host, port);
        } else {
            throw std::runtime_error("Unsupported socket type: " + type);
        }
    }

    Value open(VM &vm, const std::vector<Value> &args) {
        std::string path = args[0].as_string();
        std::string mode = args[1].as_string();
        return vm.scheduler.file_open(path, mode);
    }

    namespace io {
        Value read(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            int num_bytes = args[1].as_int();
            return vm.scheduler.io_read_num_bytes(handle, static_cast<size_t>(num_bytes));
        }

        Value read_until_delimiter(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            uint8_t delimiter = static_cast<uint8_t>(args[1].as_int());
            return vm.scheduler.io_read_until_delimiter(handle, delimiter);
        }

        Value read_line(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            return vm.scheduler.io_read_until_delimiter(handle, '\n');
        }

        Value read_all(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            return vm.scheduler.io_read_all(handle);
        }

        Value write(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            const Value &val = args[1];
            return vm.scheduler.io_write(handle, val);
        }

        Value accept(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            return vm.scheduler.io_accept(handle);
        }

        Value close(VM &vm, const std::vector<Value> &args) {
            auto handle = args[0].as_io_handle();
            vm.scheduler.io_close(handle);
            return {};
        }
    }

    Value pack(VM &vm, const std::vector<Value> &args) {
        const Value &value = args[0];
        std::string specifier = args[1].as_string();

        if (specifier == "i8") {
            int8_t v = static_cast<int8_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(std::vector<uint8_t>{static_cast<uint8_t>(v)});
        }
        if (specifier == "u8") {
            uint8_t v = static_cast<uint8_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(std::vector<uint8_t>{v});
        }
        if (specifier == "i16") {
            int16_t v = static_cast<int16_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(static_cast<uint16_t>(v)));
        }
        if (specifier == "u16") {
            uint16_t v = static_cast<uint16_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(v));
        }
        if (specifier == "i32") {
            int32_t v = static_cast<int32_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(static_cast<uint32_t>(v)));
        }
        if (specifier == "u32") {
            uint32_t v = static_cast<uint32_t>(value.as_int());
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(v));
        }
        if (specifier == "f32") {
            float v = static_cast<float>(value.as_float());
            uint32_t bits = 0;
            std::memcpy(&bits, &v, sizeof(v));
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(bits));
        }
        if (specifier == "f64") {
            double v = value.as_float();
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(v));
            return vm.heap->allocate<ByteArray>(to_little_endian_bytes(bits));
        }

        throw std::runtime_error("Unsupported specifier: " + specifier);
    }

    Value unpack(VM &, const std::vector<Value> &args) {
        std::vector<uint8_t> bytes = args[0].as_byte_array()->data;
        std::string specifier = args[1].as_string();

        size_t expected_size = specifier_size(specifier);
        if (bytes.size() != expected_size) {
            throw std::runtime_error(
                "unpack expected " + std::to_string(expected_size) +
                " byte(s) for specifier '" + specifier + "' but got " + std::to_string(bytes.size())
            );
        }

        if (specifier == "i8") {
            return static_cast<int>(static_cast<int8_t>(bytes[0]));
        }
        if (specifier == "u8") {
            return static_cast<int>(bytes[0]);
        }
        if (specifier == "i16") {
            uint16_t u = from_little_endian_bytes<uint16_t>(bytes);
            return static_cast<int>(static_cast<int16_t>(u));
        }
        if (specifier == "u16") {
            uint16_t u = from_little_endian_bytes<uint16_t>(bytes);
            return static_cast<int>(u);
        }
        if (specifier == "i32") {
            uint32_t u = from_little_endian_bytes<uint32_t>(bytes);
            return static_cast<int>(static_cast<int32_t>(u));
        }
        if (specifier == "u32") {
            uint32_t u = from_little_endian_bytes<uint32_t>(bytes);
            return static_cast<int>(u);
        }
        if (specifier == "f32") {
            uint32_t bits = from_little_endian_bytes<uint32_t>(bytes);
            float v = 0.0f;
            std::memcpy(&v, &bits, sizeof(v));
            return static_cast<double>(v);
        }
        if (specifier == "f64") {
            uint64_t bits = from_little_endian_bytes<uint64_t>(bytes);
            double v = 0.0;
            std::memcpy(&v, &bits, sizeof(v));
            return v;
        }

        throw std::runtime_error("Unsupported specifier: " + specifier);
    }
}
