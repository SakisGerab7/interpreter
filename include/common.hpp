#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <variant>
#include <ctime>
#include <cmath>
#include <functional>
#include <chrono>

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;