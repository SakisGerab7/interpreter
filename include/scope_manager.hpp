#pragma once

#include "common.hpp"
#include "codegen.hpp"

struct ScopeManager {
    struct Local {
        std::string name;
        int depth;
        bool is_captured;

        Local(const std::string &name) : name(name), depth(-1), is_captured(false) {}
    };

    struct Upvalue {
        uint8_t index;
        bool is_local;

        Upvalue(uint8_t index, bool is_local) : index(index), is_local(is_local) {}
    };

    std::shared_ptr<ScopeManager> parent;
    std::vector<Local> locals;
    std::vector<Upvalue> upvalues;
    int scope_depth = 0;

    ScopeManager(std::shared_ptr<ScopeManager> parent = nullptr, bool is_method = false) : parent(parent) {
        if (is_method) {
            locals.emplace_back("self");
            locals.back().depth = 0;
        } else {
            locals.emplace_back(""); // slot 0 is reserved for VM internal use
        }
    }

    int depth() const { return scope_depth; }

    void begin_scope() {
        scope_depth++;
    }

    void end_scope(std::function<void(bool)> emit_func) {
        scope_depth--;
        while (!locals.empty() && locals.back().depth > scope_depth) {
            if (emit_func) {
                emit_func(locals.back().is_captured);
            }
            locals.pop_back();
        }
    }

    void declare_variable(const Token &name) {
        if (scope_depth == 0) return; // global variable

        for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
            if (locals[i].depth != -1 && locals[i].depth < scope_depth) {
                break;
            }
            if (locals[i].name == name.value) {
                throw std::runtime_error("Variable with this name already declared in this scope: " + name.value);
            }
        }

        locals.emplace_back(name.value);
    }

    void mark_initialized() {
        if (scope_depth == 0) return;
        locals.back().depth = scope_depth;
    }

    int resolve_local(const Token &name) {
        for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
            if (locals[i].name == name.value) {
                if (locals[i].depth != -1) return i;
                throw std::runtime_error("Cannot read local variable in its own initializer: " + name.value);
            }
        }

        return -1;
    }

    int add_upvalue(uint8_t index, bool is_local) {
        int upvalue_count = static_cast<int>(upvalues.size());
        for (int i = 0; i < upvalue_count; i++) {
            if (upvalues[i].index == index && upvalues[i].is_local == is_local) {
                return i;
            }
        }

        if (upvalue_count == 255) {
            throw std::runtime_error("Too many closure upvalues");
        }

        upvalues.emplace_back(index, is_local);
        return upvalue_count;
    }

    int resolve_upvalue(const Token &name) {
        if (!parent) return -1;

        if (int local = parent->resolve_local(name); local != -1) {
            parent->locals[local].is_captured = true;
            return add_upvalue(static_cast<uint8_t>(local), true);
        }

        if (int upvalue = parent->resolve_upvalue(name); upvalue != -1) {
            return add_upvalue(static_cast<uint8_t>(upvalue), false);
        }

        return -1;
    }

    enum class VarType { Local, Upvalue, Global };

    struct ResolveResult {
        VarType type;
        int index;
    };

    ResolveResult resolve_variable(const Token &name) {
        if (int local = resolve_local(name); local != -1) {
            return { VarType::Local, local };
        }

        if (int upvalue = resolve_upvalue(name); upvalue != -1) {
            return { VarType::Upvalue, upvalue };
        }

        return { VarType::Global, -1 };
    }
};