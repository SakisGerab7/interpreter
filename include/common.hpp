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
#include <cassert>
#include <memory>
#include <variant>
#include <ctime>
#include <cmath>
#include <functional>
#include <chrono>
#include <any>
#include <thread>

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;