#include "vm.hpp"

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

    if (threads.erase(thread->ID) != 0) {
        std::cerr << "[Killing thread " << thread->ID << "]\n";
    }

    for (auto &child : thread->children) {
        kill_thread_and_children(child);
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
            std::this_thread::sleep_for(sleep_duration);
        }
    }
}

Value Scheduler::schedule(VM &vm) {
    Value final_return;

    while (!threads.empty()) {
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

        // print return values
        std::cerr << "[Return Values: ";
        for (const auto& [tid, ret_val] : return_values) {
            std::cerr << "Thread " << tid << ": " << ret_val.to_string() << " ";
        }
        std::cerr << "]\n";

        auto now = std::chrono::steady_clock::now();
        wake_threads(now);

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


