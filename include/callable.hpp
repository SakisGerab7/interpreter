#pragma once

#include "common.hpp"
#include "interpreter.hpp"

using FnType = std::function<Value(Interpreter&, const std::vector<Value>&)>;

struct Callable {
    size_t Arity;
    FnType Fn;
    const FunctionStmt *Declaration = nullptr;

    Callable(size_t arity, FnType fn) : Arity(arity), Fn(std::move(fn)) {}
    Callable(const FunctionStmt *decl) : Declaration(decl) { Arity = Declaration->Params.size(); }

    Value call(Interpreter &interp, const std::vector<Value> &args);
};
