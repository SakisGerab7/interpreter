#include "green_thread.hpp"
#include "pipe.hpp"

// Out-of-line constructor and destructor to avoid circular dependency issues
GreenThread::GreenThread(size_t id, Closure* closure)
    : Object(Type::Thread), ID(id), active_select(nullptr)
{
    if (closure) {
        ctx.stack[ctx.stack_size++] = Value(closure);
        ctx.frames.emplace_back(closure, 0, static_cast<int>(ctx.stack_size) - 1);
    }
}

GreenThread::~GreenThread() = default;

size_t GreenThread::object_size() const {
    size_t size = sizeof(GreenThread);
    size += ctx.frames.capacity() * sizeof(CallFrame);
    size += ctx.open_upvalues.capacity() * sizeof(Upvalue*);
    size += joiners.capacity() * sizeof(GreenThread*);
    size += children.capacity() * sizeof(GreenThread*);
    if (active_select) {
        size += sizeof(SelectFrame);
        size += active_select->cases.capacity() * sizeof(SelectCase);
    }
    return size;
}
