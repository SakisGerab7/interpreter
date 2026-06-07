#pragma once

#include "value.hpp"

struct VM;

namespace native_functions {
    Value cli_args(VM &vm, const std::vector<Value> &args);
    Value clock(VM &, const std::vector<Value> &args);
    Value len(VM &, const std::vector<Value> &args);
    Value str(VM &, const std::vector<Value> &args);
    Value int_fn(VM &, const std::vector<Value> &args);
    Value float_fn(VM &, const std::vector<Value> &args);
    Value type(VM &, const std::vector<Value> &args);

    namespace math {
        Value pow(VM &, const std::vector<Value> &args);
        Value abs(VM &, const std::vector<Value> &args);
        Value round(VM &, const std::vector<Value> &args);
        Value sqrt(VM &, const std::vector<Value> &args);
        Value sin(VM &, const std::vector<Value> &args);
        Value cos(VM &, const std::vector<Value> &args);
        Value tan(VM &, const std::vector<Value> &args);
        Value floor(VM &, const std::vector<Value> &args);
        Value ceil(VM &, const std::vector<Value> &args);
        Value min(VM &, const std::vector<Value> &args);
        Value max(VM &, const std::vector<Value> &args);
        Value srand(VM &, const std::vector<Value> &args);
        Value rand(VM &, const std::vector<Value>&);
        Value randint(VM &, const std::vector<Value> &args);
        Value asin(VM &, const std::vector<Value> &args);
        Value acos(VM &, const std::vector<Value> &args);
        Value atan(VM &, const std::vector<Value> &args);
        Value log2(VM &, const std::vector<Value> &args);
        Value log10(VM &, const std::vector<Value> &args);
        Value ln(VM &, const std::vector<Value> &args);
        Value exp(VM &, const std::vector<Value> &args);
    }
    
    namespace string {
        Value to_upper(VM &vm, const std::vector<Value> &args);
        Value to_lower(VM &vm, const std::vector<Value> &args);
        Value trim(VM &vm, const std::vector<Value> &args);
        Value split(VM &vm, const std::vector<Value> &args);
        Value to_byte_array(VM &vm, const std::vector<Value> &args);
    }
    
    namespace byte_array {
        Value to_string(VM &vm, const std::vector<Value> &args);
    }
    
    namespace array {
        Value arange(VM &vm, const std::vector<Value> &args);
        Value push(VM &vm, const std::vector<Value> &args);
        Value pop(VM &vm, const std::vector<Value> &args);
        Value shift(VM &vm, const std::vector<Value> &args);
        Value unshift(VM &vm, const std::vector<Value> &args);
        Value slice(VM &vm, const std::vector<Value> &args);
        Value sum(VM &vm, const std::vector<Value> &args);
        Value foreach(VM &vm, const std::vector<Value> &args);
        Value map(VM &vm, const std::vector<Value> &args);
    }

    Value sleep(VM &vm, const std::vector<Value> &args);
    Value thread_id(VM &vm, const std::vector<Value> &args);
    Value join(VM &vm, const std::vector<Value> &args);
    Value detach(VM &vm, const std::vector<Value> &args);
    Value pipe(VM &vm, const std::vector<Value> &args);
    Value listen(VM &vm, const std::vector<Value> &args);
    Value connect(VM &vm, const std::vector<Value> &args);
    Value open(VM &vm, const std::vector<Value> &args);

    namespace io {
        Value read(VM &vm, const std::vector<Value> &args);
        Value read_line(VM &vm, const std::vector<Value> &args);
        Value read_all(VM &vm, const std::vector<Value> &args);
        Value read_until_delimiter(VM &vm, const std::vector<Value> &args);
        Value accept(VM &vm, const std::vector<Value> &args);
        Value write(VM &vm, const std::vector<Value> &args);
        Value close(VM &vm, const std::vector<Value> &args);
    }

    Value pack(VM &, const std::vector<Value> &args);
    Value unpack(VM &, const std::vector<Value> &args);
}