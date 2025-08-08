#include "callable.hpp"
#include "return_exception.hpp"

Value Callable::call(Interpreter &interp, const std::vector<Value> &args) {
    if (args.size() != Arity) {
        throw std::runtime_error("Incorrect number of arguments");
    }

    if (!Declaration) return Fn(interp, args);

    std::shared_ptr<Environment> env = std::make_shared<Environment>(interp.Globals.get());

    for (size_t i = 0; i < Arity; i++) {
        env->define(Declaration->Params[i].Value, args[i]);
    }

    try {
        interp.execute_block(std::get<BlockStmt>(*Declaration->Body), env);
    } catch (const ReturnException &e) {
        return e.Val;
    }

    return Value();
}