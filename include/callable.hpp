#pragma once

#include "common.hpp"
#include "interpreter.hpp"

struct Callable {
    using FnType = std::function<Value(Interpreter&, const std::vector<Value>&)>;

    size_t Arity;
    FnType Fn = nullptr;

    std::string_view Name;
    std::vector<Token> Params;
    std::vector<Stmt *> Body;
    std::shared_ptr<Environment> Closure;

    Callable(size_t arity, FnType fn)
        : Arity(arity), Fn(std::move(fn)) {}

    Callable(std::string_view name, const std::vector<Token> &params, const std::vector<Stmt *> &body, std::shared_ptr<Environment> closure)
        : Name(name), Params(params), Body(body), Closure(std::move(closure)), Arity(params.size()) {}

    std::string to_string() const {
        if (Fn) return "<native function>";
        std::stringstream ss;
        ss << "<function " << Name << "/" << Arity << ">";
        return ss.str();
    }
};
