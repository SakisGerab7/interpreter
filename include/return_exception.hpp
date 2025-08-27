#pragma once

#include "common.hpp"
#include "value.hpp"

struct ReturnException : public std::runtime_error {
    Value Val;

    ReturnException(Value value) : Val(value), std::runtime_error("return " + value.to_string()) {}
};