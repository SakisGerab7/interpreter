#include "vm.hpp"
#include "threading.hpp"

GreenThread::Ptr Scheduler::get_thread_by_id(size_t id) {
    auto it = threads.find(id);
    return it != threads.end() ? it->second : nullptr;
}

void Scheduler::add_thread(GreenThread::Ptr thread) {
    threads[thread->ID] = thread;
}

void Scheduler::enqueue(GreenThread::Ptr thread) {
    ready_queue.push_back(thread->ID);
}

GreenThread::Ptr Scheduler::dequeue() {
    if (ready_queue.empty()) return nullptr;
    size_t tid = ready_queue.front();
    ready_queue.pop_front();
    return threads[tid];
}

void Scheduler::block_thread(GreenThread::Ptr &thread) {
    blocked_queue.emplace(thread->wake_time, thread->ID);
}

Value Scheduler::get_return_value(size_t thread_id) {
    if (auto it = return_values.find(thread_id); it != return_values.end()) {
        return it->second;
    }

    return {};
}

void Scheduler::set_return_value(GreenThread::Ptr &thread, const Value &return_value) {
    return_values[thread->ID] = return_value;
}

Pipe::Ptr Scheduler::get_pipe_by_id(size_t id) {
    auto it = pipes.find(id);
    return it != pipes.end() ? it->second : nullptr;
}

void Scheduler::notify_pipe_select_waiters(Pipe::Ptr &pipe) {
    for (auto &thread : pipe->select_waiters) {
        if (thread->state == GreenThread::Blocked) {
            thread->state = GreenThread::Ready;
            enqueue(thread);
        }
    }

    pipe->select_waiters.clear();
}

void Scheduler::send_to_pipe(GreenThread::Ptr &current_thread, Pipe::Ptr pipe, const Value &val) {
    if (pipe->closed) {
        throw std::runtime_error("Cannot send to a closed pipe");
    }

    // direct handoff to a waiting reader
    if (!pipe->readers.empty()) {
        auto reader = pipe->readers.front();
        pipe->readers.pop_front();

        reader->stack[reader->stack_size - 1] = val;
        reader->state = GreenThread::Ready;
        enqueue(reader);

        notify_pipe_select_waiters(pipe);
        return;
    }

    // buffer has space
    if (pipe->buffer.size() < pipe->capacity) {
        pipe->buffer.push_back(val);
        notify_pipe_select_waiters(pipe);
        return;
    }

    // Block the current thread
    pipe->writers.push_back(current_thread);
    current_thread->state = GreenThread::Blocked;
    current_thread->wake_time = {};
    current_thread->pending_value = val;
}

Value Scheduler::receive_from_pipe(GreenThread::Ptr current_thread, Pipe::Ptr pipe) {
    if (!pipe->buffer.empty()) {
        Value val = pipe->buffer.front();
        pipe->buffer.pop_front();

        // wake up a waiting writer if any
        if (!pipe->writers.empty()) {
            auto writer = pipe->writers.front();
            pipe->writers.pop_front();

            pipe->buffer.push_back(writer->pending_value);

            writer->state = GreenThread::Ready;
            enqueue(writer);

            notify_pipe_select_waiters(pipe);
        }

        return val;
    }

    // direct handoff from a waiting writer
    if (!pipe->writers.empty()) {
        auto writer = pipe->writers.front();
        pipe->writers.pop_front();

        Value val = writer->pending_value;

        writer->state = GreenThread::Ready;
        enqueue(writer);

        notify_pipe_select_waiters(pipe);
        return val;
    }

    if (pipe->closed) {
        return {}; // return null value on closed pipe
    }

    // Block the current thread
    pipe->readers.push_back(current_thread);
    current_thread->state = GreenThread::Blocked;
    current_thread->wake_time = {};
    return {};
}

void Scheduler::close_pipe(Pipe::Ptr pipe) {
    pipe->closed = true;

    // Wake up all waiting readers with null values
    while (!pipe->readers.empty()) {
        auto reader = pipe->readers.front();
        pipe->readers.pop_front();

        reader->stack[reader->stack_size - 1] = {}; // null value
        reader->state = GreenThread::Ready;
        enqueue(reader);
    }

    pipe->readers.clear();

    // Wake up all waiting writers with an error
    while (!pipe->writers.empty()) {
        auto writer = pipe->writers.front();
        pipe->writers.pop_front();

        throw std::runtime_error("Cannot write to a closed pipe");
    }

    pipe->writers.clear();

    notify_pipe_select_waiters(pipe);
}

void Scheduler::select_begin(GreenThread::Ptr thread, uint8_t case_count) {

    SelectFrame select_frame;
    select_frame.cases.reserve(case_count);

    thread->active_select = std::make_unique<SelectFrame>(select_frame);
}

void Scheduler::select_add_recv_case(GreenThread::Ptr thread, Pipe::Ptr pipe, uint16_t target_ip, uint8_t slot) {
    SelectCase select_case;
    select_case.type = SelectCase::Recv;
    select_case.pipe = pipe;
    select_case.slot = slot;
    select_case.target_ip = target_ip;

    thread->active_select->cases.push_back(select_case);
}

void Scheduler::select_add_send_case(GreenThread::Ptr thread, Pipe::Ptr pipe, uint16_t target_ip, Value val) {
    SelectCase select_case;
    select_case.type = SelectCase::Send;
    select_case.pipe = pipe;
    select_case.value = val;
    select_case.target_ip = target_ip;

    thread->active_select->cases.push_back(select_case);
}

void Scheduler::select_add_default_case(GreenThread::Ptr thread, uint16_t target_ip) {
    thread->active_select->has_default = true;
    thread->active_select->default_target_ip = target_ip;
}

void Scheduler::select_execute(GreenThread::Ptr current_thread, int &curr_ip) {
    std::vector<SelectCase*> ready_cases;
    for (auto &sel_case : current_thread->active_select->cases) {
        if ((sel_case.type == SelectCase::Send && sel_case.pipe && sel_case.pipe->can_send()) ||
            (sel_case.type == SelectCase::Recv && sel_case.pipe && sel_case.pipe->can_receive())) {
            ready_cases.push_back(&sel_case);
        }
    }

    if (!ready_cases.empty()) {
        // Randomly pick one of the ready cases
        SelectCase *selected = ready_cases[rand() % ready_cases.size()];

        if (selected->type == SelectCase::Recv) {
            Value received = receive_from_pipe(current_thread, selected->pipe);
            if (selected->slot != 0xFF) {
                current_thread->stack[selected->slot] = received;
            }
        } else if (selected->type == SelectCase::Send) {
            send_to_pipe(current_thread, selected->pipe, selected->value);
        }

        curr_ip = selected->target_ip;
        current_thread->active_select = nullptr;
        return;
    }

    // No ready cases, jump to default if exists
    if (current_thread->active_select->has_default) {
        curr_ip = current_thread->active_select->default_target_ip;
        current_thread->active_select = nullptr;
        return;
    }

    // No default case, block the thread
    for (const auto &sel_case : current_thread->active_select->cases) {
        if (!sel_case.pipe) continue; // skip disabled cases
        sel_case.pipe->select_waiters.push_back(current_thread);
    }

    current_thread->state = GreenThread::Blocked;
    curr_ip -= 1; // stay on the SELECT_EXEC instruction
}

bool Pipe::can_receive() {
    return !buffer.empty() || !writers.empty() || closed;
}

bool Pipe::can_send() {
    return !closed && (!readers.empty() || buffer.size() < capacity);
}

void Scheduler::notify_waiters(GreenThread::Ptr &thread) {
    for (auto it = join_map.begin(); it != join_map.end(); ) {
        if (it->second == thread->ID) {
            auto parent_thread = get_thread_by_id(it->first);
            if (parent_thread && parent_thread->state != GreenThread::Finished) {
                parent_thread->stack[parent_thread->stack_size - 1] = get_return_value(thread->ID);
                parent_thread->state = GreenThread::Ready;
                // put to the start of the ready queue
                ready_queue.push_front(parent_thread->ID);
            }
            it = join_map.erase(it);
        } else {
            ++it;
        }
    }
}

void Scheduler::kill_thread_and_children(GreenThread::Ptr &thread) {
    if (threads.erase(thread->ID) != 0) {
        std::cerr << "[Killing thread " << thread->ID << "]\n";
    }

    for (auto &child : thread->children) {
        kill_thread_and_children(child);
    }
}

void Scheduler::wake_threads(const std::chrono::steady_clock::time_point &now) {
    while (!blocked_queue.empty() && blocked_queue.top().first <= now) {
        auto thread_id = blocked_queue.top().second;
        blocked_queue.pop();
        
        auto thread_it = threads.find(thread_id);
        if (thread_it == threads.end()) continue; // Thread might have finished

        auto thread = thread_it->second;
        thread->state = GreenThread::Ready;
        thread->wake_time = {};
        enqueue(thread);
    }
}

void Scheduler::send_to_sleep(GreenThread::Ptr thread, int ms) {
    // std::cout << "[Thread " << thread->ID << " sleeping for " << ms << " ms]\n";
    if (ms <= 0) {
        thread->state = GreenThread::Ready;
        return;
    }

    thread->state = GreenThread::Blocked;
    thread->wake_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
}

void Scheduler::sleep_until_ready(const std::chrono::steady_clock::time_point &now) {
    if (!blocked_queue.empty()) {
        auto sleep_duration = blocked_queue.top().first - now;
        if (sleep_duration.count() > 0) {
            std::cerr << "[Scheduler sleeping for " 
                      << std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count() 
                      << " ms]\n";
            std::this_thread::sleep_for(sleep_duration);
        }
    }
}

Value Scheduler::schedule(VM &vm) {
    Value final_return;

    while (!threads.empty()) {
        auto now = std::chrono::steady_clock::now();
        wake_threads(now);

        // print all threads
        for (const auto& [id, thread] : threads) {
            std::cerr << "[Thread " << id << " State: ";
            switch (thread->state) {
                case GreenThread::Running:  std::cerr << "Running"; break;
                case GreenThread::Ready:    std::cerr << "Ready"; break;
                case GreenThread::Blocked:  std::cerr << "Blocked"; break;
                case GreenThread::Finished: std::cerr << "Finished"; break;
            }
            std::cerr << "]\n";
        }

        // print ready queue
        std::cerr << "[Ready Queue: ";
        for (const auto& tid : ready_queue) {
            std::cerr << tid << " ";
        }
        std::cerr << "]\n";

        // print blocked queue
        std::cerr << "[Blocked Queue: ";
        auto blocked_copy = blocked_queue;
        while (!blocked_copy.empty()) {
            std::cerr << "(" << blocked_copy.top().second << ", " 
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                          blocked_copy.top().first.time_since_epoch()).count() 
                      << "ms) ";
            blocked_copy.pop();
        }
        std::cerr << "]\n";

        auto next_thread = dequeue();
        if (!next_thread) {
            // std::cout << "[No ready threads, scheduler sleeping]\n";
            sleep_until_ready(now);
            continue;
        }

        // std::cout << "[Scheduling thread " << next_thread->ID << "]\n";

        next_thread->state = GreenThread::Running;

        vm.current_thread = next_thread;
        vm.run();
        vm.current_thread = nullptr;
        
        final_return = get_return_value(next_thread->ID);

        if (next_thread->state == GreenThread::Finished) {
            notify_waiters(next_thread);
            kill_thread_and_children(next_thread);
            continue;
        }
        
        if (next_thread->state == GreenThread::Blocked) {
            if (next_thread->wake_time == std::chrono::steady_clock::time_point{}) {
                // Blocked without a wake time (e.g., waiting for join)
                continue;
            }

            block_thread(next_thread);
            continue;
        }

        next_thread->state = GreenThread::Ready;
        enqueue(next_thread);
    }

    return final_return;
}


