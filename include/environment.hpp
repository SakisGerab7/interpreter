#pragma once

#include "common.hpp"
#include "value.hpp"
#include "token.hpp"

struct Environment {
    std::unordered_map<std::string_view, Value> Variables;
    Environment *Parent = nullptr;

    Environment() = default;

    Environment(Environment *parent) : Parent(parent) {}

    void define(std::string_view name, const Value &val) {
        Variables[name] = val;
    }

    void assign(const Token &token, const Value &val) {
        auto it = Variables.find(token.Value);
        if (it != Variables.end()) {
            it->second = val;
            return;
        }

        if (Parent) {
            Parent->assign(token, val);
            return;
        }

        throw std::runtime_error("Undefined variable '" + std::string(token.Value) + "'.");
    }

    const Value &get(const Token &token) const {
        auto it = Variables.find(token.Value);
        if (it != Variables.end()) return it->second;

        if (Parent) return Parent->get(token);

        throw std::runtime_error("Undefined variable '" + std::string(token.Value) + "'.");
    }
};