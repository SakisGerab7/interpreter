#include "pipe.hpp"
#include "vm.hpp"

bool Pipe::can_receive() {
    return !buffer.empty() || !writers.empty() || closed;
}

bool Pipe::can_send() {
    return !closed && (!readers.empty() || buffer.size() < capacity);
}

void Pipe::notify_selectors(VM &vm) {
    for (auto &thread : selectors) {
        if (thread->state == GreenThread::Blocked) {
            std::cerr << "[Notifying thread " << thread->ID << " waiting on select for pipe " << ID << "]\n";
            thread->state = GreenThread::Ready;
            vm.scheduler.enqueue(thread);
        }
    }

    selectors.clear();
}

void Pipe::send(const Value &value, VM &vm) {
    if (closed) {
        throw std::runtime_error("Cannot send to a closed pipe");
    }

    // notify any waiting select cases before blocking
    notify_selectors(vm);

    // direct handoff to a waiting reader
    if (!readers.empty()) {
        auto reader = readers.front();
        readers.pop_front();

        reader->ctx.poke_stack(value);
        reader->state = GreenThread::Ready;
        vm.scheduler.enqueue(reader);
        return;
    }

    // buffer has space
    if (buffer.size() < capacity) {
        buffer.push_back(value);
        return;
    }

    // Block the current thread
    writers.push_back(vm.current_thread);
    vm.current_thread->state = GreenThread::Blocked;
    vm.current_thread->wake_time = {};
    vm.current_thread->pending_value = value;
}

Value Pipe::recv(VM &vm) {
    if (!buffer.empty()) {
        Value val = buffer.front();
        buffer.pop_front();

        // wake up a waiting writer if any
        if (!writers.empty()) {
            auto writer = writers.front();
            writers.pop_front();

            buffer.push_back(writer->pending_value);

            writer->state = GreenThread::Ready;
            vm.scheduler.enqueue(writer);

            notify_selectors(vm);
        }

        return val;
    }

    // direct handoff from a waiting writer
    if (!writers.empty()) {
        auto writer = writers.front();
        writers.pop_front();

        Value val = writer->pending_value;

        writer->state = GreenThread::Ready;
        vm.scheduler.enqueue(writer);

        notify_selectors(vm);
        return val;
    }

    if (closed) {
        return {}; // return null value on closed pipe
    }

    // Block the current thread
    readers.push_back(vm.current_thread);
    vm.current_thread->state = GreenThread::Blocked;
    vm.current_thread->wake_time = {};
    notify_selectors(vm);
    return {};
}

void Pipe::close(VM &vm) {
    closed = true;

    // Wake up all waiting readers with null values
    while (!readers.empty()) {
        auto reader = readers.front();
        readers.pop_front();

        reader->ctx.poke_stack({}); // null value
        reader->state = GreenThread::Ready;
        vm.scheduler.enqueue(reader);
    }

    readers.clear();

    // Wake up all waiting writers with an error
    while (!writers.empty()) {
        auto writer = writers.front();
        writers.pop_front();

        throw std::runtime_error("Cannot write to a closed pipe");
    }

    writers.clear();

    notify_selectors(vm);
}

SelectFrame::SelectFrame(size_t case_count) {
    cases.reserve(case_count);
}

void SelectFrame::add_recv_case(Pipe* pipe, uint16_t target_ip, uint8_t slot) {
    SelectCase select_case;
    select_case.type = SelectCase::Recv;
    select_case.pipe = pipe;
    select_case.slot = slot;
    select_case.target_ip = target_ip;
    cases.push_back(select_case);
}

void SelectFrame::add_send_case(Pipe* pipe, uint16_t target_ip, Value val) {
    SelectCase select_case;
    select_case.type = SelectCase::Send;
    select_case.pipe = pipe;
    select_case.value = val;
    select_case.target_ip = target_ip;
    cases.push_back(select_case);
}

void SelectFrame::add_default_case(uint16_t target_ip) {
    has_default = true;
    default_target_ip = target_ip;
}

bool SelectFrame::execute(VM &vm, int &curr_ip) {
    std::vector<SelectCase*> ready_cases;
    for (auto &sel_case : cases) {
        if (sel_case.pipe) {
            if (sel_case.type == SelectCase::Recv && sel_case.pipe->can_receive()) {
                ready_cases.push_back(&sel_case);
            } else if (sel_case.type == SelectCase::Send && sel_case.pipe->can_send()) {
                ready_cases.push_back(&sel_case);
            }
        }
    }

    if (!ready_cases.empty()) {
        // Randomly pick one of the ready cases
        SelectCase *selected = ready_cases[rand() % ready_cases.size()];

        if (selected->type == SelectCase::Recv) {
            Value received = selected->pipe->recv(vm);
            if (selected->slot != 0xFF) {
                vm.current_thread->ctx.stack[selected->slot] = received;
            }
        } else if (selected->type == SelectCase::Send) {
            selected->pipe->send(selected->value, vm);
        }

        curr_ip = selected->target_ip;
        return true;
    }

    // No ready cases, jump to default if exists
    if (has_default) {
        curr_ip = default_target_ip;
        return true;
    }

    // No default case, block the thread
    for (const auto &sel_case : cases) {
        if (!sel_case.pipe) continue; // skip disabled cases
        sel_case.pipe->selectors.push_back(vm.current_thread);
    }

    vm.current_thread->state = GreenThread::Blocked;
    curr_ip -= 1; // stay on the SELECT_EXEC instruction
    return false;
}
