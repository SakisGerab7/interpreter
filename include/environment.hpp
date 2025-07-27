#pragma once

#include "common.hpp"
#include "value.hpp"
#include "token.hpp"

struct Environment {
    std::unordered_map<std::string, Value> variables;
    Environment *Parent = nullptr;

    Environment() = default;

    Environment(Environment *parent) : Parent(parent) {}

    void define(const std::string &name, const Value &val) {
        variables[name] = val;
    }

    void assign(const Token &token, const Value &val) {
        auto it = variables.find(token.Value);
        if (it != variables.end()) {
            it->second = val;
            return;
        }

        if (Parent) {
            Parent->assign(token, val);
            return;
        }

        throw std::runtime_error("Undefined variable '" + token.Value + "'.");
    }

    const Value &get(const Token &token) const {
        auto it = variables.find(token.Value);
        if (it != variables.end()) return it->second;

        if (Parent) return Parent->get(token);

        throw std::runtime_error("Undefined variable '" + token.Value + "'.");
    }
};