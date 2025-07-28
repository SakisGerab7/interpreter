#pragma once

#include "common.hpp"
#include "value.hpp"

class Interpreter;

using FnType = std::function<Value(Interpreter&, const std::vector<Value>&)>;

struct Callable {
    size_t Arity;
    FnType Fn;

    Callable(size_t arity, FnType fn) : Arity(arity), Fn(std::move(fn)) {}

    Value call(Interpreter& interp, const std::vector<Value>& args) {
        if (args.size() != Arity) {
            throw std::runtime_error("Incorrect number of arguments");
        }

        return Fn(interp, args);
    }
};