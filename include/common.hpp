#pragma once

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <queue>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <memory>
#include <variant>
#include <ctime>
#include <cmath>
#include <functional>
#include <chrono>
#include <any>
#include <thread>
#include <stdexcept>
#include <filesystem>


#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

namespace vm_signal {
    inline volatile sig_atomic_t suspend_requested = 0;
}
