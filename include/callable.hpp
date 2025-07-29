#pragma once

#include "common.hpp"
#include "value.hpp"
#include "environment.hpp"

class Interpreter;

using FnType = std::function<Value(Interpreter&, const std::vector<Value>&)>;

struct Callable {
    size_t Arity;
    FnType Fn;
    const FunctionStmt *Declaration;

    Callable(size_t arity, FnType fn) : Arity(arity), Fn(std::move(fn)) {}
    Callable(const FunctionStmt *decl) : Declaration(decl) { Arity = Declaration->Params.size(); }

    Value call(Interpreter &interp, const std::vector<Value> &args) {
        if (args.size() != Arity) {
            throw std::runtime_error("Incorrect number of arguments");
        }

        if (!Declaration) return Fn(interp, args);

        std::shared_ptr<Environment> env = std::make_shared<Environment>(interp.Globals.get());

        // std::cout << "inside function " << Declaration->Name << "(";
        for (size_t i = 0; i < Arity; i++) {
            env->define(Declaration->Params[i].Value, args[i]);
            // std::cout << " " << Declaration->Params[i].Value << "=" << args[i].to_string() << " ";
        }

        // std::cout << ")\n";

        interp.execute_block(*dynamic_cast<BlockStmt*>(Declaration->Body.get()), env);
        
        return 0;
    }
};