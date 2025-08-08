#pragma once

#include "common.hpp"
#include "value.hpp"

struct ReturnException : public std::runtime_error {
    Value Val;

    ReturnException(Value val) : Val(val), std::runtime_error("return " + val.to_string()) {}
};