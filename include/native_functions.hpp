#pragma once

#include "value.hpp"

namespace native_functions {
    Value clock(VM &, const std::vector<Value> &args) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis) / 1000.0;
    }

    Value len(VM &, const std::vector<Value> &args) {
        if (args[0].is_array())
            return static_cast<int>(args[0].as_array()->size());
        if (args[0].is_object())
            return static_cast<int>(args[0].as_object()->size());
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
        if (v.is_object())   return "object";
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

        Value split(VM &, const std::vector<Value> &args) {
            std::string s = args[0].as_string();
            std::string delimiter = args[1].as_string();
            std::vector<Value> tokens;
            size_t pos = 0;
            while ((pos = s.find(delimiter)) != std::string::npos) {
                tokens.push_back(s.substr(0, pos));
                s.erase(0, pos + delimiter.length());
            }

            tokens.push_back(s);
            return std::make_shared<Array>(tokens);
        }
    }

    namespace array {
        Value arange(VM &, const std::vector<Value> &args) {
            int start = args[0].as_int();
            int end = args[1].as_int();
            int step = args[2].as_int();
            if (step == 0) {
                throw std::runtime_error("Step cannot be zero");
            }
            
            std::vector<Value> result;
            if ((step > 0 && start >= end) || (step < 0 && start <= end)) {
                return std::make_shared<Array>(result);
            }

            for (int i = start; (step > 0 ? i < end : i > end); i += step) {
                result.push_back(i);
            }

            return std::make_shared<Array>(result);
        }

        Value push(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            arr->elements.push_back(args[1]);
            return {};
        }

        Value pop(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            if (arr->elements.empty()) throw std::runtime_error("Cannot pop from an empty array");
            Value val = arr->elements.back();
            arr->elements.pop_back();
            return val;
        }

        Value shift(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            if (arr->elements.empty()) throw std::runtime_error("Cannot shift from an empty array");
            Value val = arr->elements.front();
            arr->elements.erase(arr->elements.begin());
            return val;
        }

        Value unshift(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            arr->elements.insert(arr->elements.begin(), args[1]);
            return {};
        }

        Value slice(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            int start = args[1].as_int();
            int end = args[2].as_int();

            if (start < 0 || end > static_cast<int>(arr->elements.size()) || start > end) {
                throw std::runtime_error("Invalid slice indices");
            }

            std::vector<Value> sliced_elements(arr->elements.begin() + start, arr->elements.begin() + end);
            return std::make_shared<Array>(sliced_elements);
        }

        Value sum(VM &, const std::vector<Value> &args) {
            auto arr = args[0].as_array();
            Value total = 0.0;

            for (const auto &elem : arr->elements) {
                total = total + elem;
            }
            
            return total;
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
        ThreadHandle handle = args[0].as_thread_handle();
        size_t thread_id = handle.ID;

        auto thread = vm.scheduler.get_thread_by_id(thread_id);

        // If the thread is already finished, return its return value immediately
        if (!thread || thread->state == GreenThread::Finished) {
            return vm.scheduler.get_return_value(thread_id);
        }

        // Map the current thread to the target thread for joining
        vm.scheduler.join_map[vm.current_thread->ID] = thread_id;

        // Block the current thread
        vm.current_thread->state = GreenThread::Blocked;
        vm.current_thread->wake_time = {};

        return {};
    }

    // Value detach(VM &vm, const std::vector<Value> &args) {
    //     ThreadHandle handle = args[0].as_thread_handle();
    //     size_t thread_id = handle.ID;

    //     auto thread = vm.scheduler.get_thread_by_id(thread_id);
    //     if (!thread) {
    //         throw std::runtime_error("Invalid thread handle");
    //     }

    //     // If already finished, just remove it
    //     if (thread->state == GreenThread::Finished) {
    //         vm.scheduler.kill_thread_and_children(thread);
    //         return {};
    //     }

    //     // Mark the thread as detached so it won't be joined later
    //     thread->detached = true;

    //     // Remove parent - child relationship if any
    //     vm.current_thread->children.erase(
    //         std::remove_if(
    //             vm.current_thread->children.begin(),
    //             vm.current_thread->children.end(),
    //             [thread_id](const GreenThread::Ptr &child) {
    //                 return child->ID == thread_id;
    //             }
    //         ),
    //         vm.current_thread->children.end()
    //     );

    //     return {};
    // }

    Value pipe(VM &vm, const std::vector<Value> &args) {
        int capacity = args[0].as_int();        
        size_t pipe_id = vm.scheduler.next_pipe_id++;

        auto pipe = std::make_shared<Pipe>(pipe_id, capacity);
        vm.scheduler.pipes[pipe_id] = pipe;

        return PipeHandle(pipe_id, pipe);
    }
}