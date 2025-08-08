#include "common.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "ast_printer.hpp"

#define MEM_TRACKING 1
#if MEM_TRACKING

namespace memtrack {

struct MemStats {
    size_t allocations, max_allocations;
    size_t num_bytes, max_num_bytes;
};

static MemStats stats[4];

static const char *phase_names[] = {
    "lexing",
    "parsing",
    "printing",
    "interpreting"
};

static size_t phase = 0;

void next_phase() { phase++; }

void print_stats() {
    for (size_t i = 0; i < (sizeof(stats) / sizeof(MemStats)); i++) {
        std::cout << "Phase: " << phase_names[i] << "\n"
                  << "  Max Allocations: " << stats[i].max_allocations << "\n"
                  << "  Max Bytes      : " << stats[i].max_num_bytes << "\n";
    }
}

void check_leaks() {
    for (size_t i = 0; i < sizeof(stats) / sizeof(stats[0]); ++i) {
        if (stats[i].allocations != 0 || stats[i].num_bytes != 0) {
            std::cerr << "[memtrack] WARNING: Potential memory leak ("
                      << stats[i].allocations << ", " << stats[i].num_bytes
                      << "B) in phase '" << phase_names[i] << "'\n";
        }
    }
}

void *allocate(size_t size) {
    size_t total_size = size + sizeof(size_t);
    void *raw = malloc(total_size);
    if (!raw) throw std::bad_alloc();
    *(size_t *) raw = size;

    stats[phase].allocations++;
    stats[phase].num_bytes += size;
    stats[phase].max_allocations = std::max(stats[phase].max_allocations, stats[phase].allocations);
    stats[phase].max_num_bytes = std::max(stats[phase].max_num_bytes, stats[phase].num_bytes);
    
    // std::cerr << "[" << phase_names[phase] << "]: Allocated " << size << " bytes\n";

    return (char *) raw + sizeof(size_t);
}

void deallocate(void *ptr) {
    if (!ptr) return;
    void *raw = (char *) ptr - sizeof(size_t);
    size_t size = *(size_t *)raw;
    
    stats[phase].allocations--;
    stats[phase].num_bytes -= size;
    
    free(raw);
}

}

void* operator new(size_t size) {
    return memtrack::allocate(size);
}

void operator delete(void* ptr) noexcept {
    memtrack::deallocate(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    memtrack::deallocate(ptr);
}

void* operator new[](size_t size) {
    return memtrack::allocate(size);
}

void operator delete[](void* ptr) noexcept {
    memtrack::deallocate(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    memtrack::deallocate(ptr);
}

#endif

bool run(const std::string &src) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;

    std::vector<Token> tokens;
    tokens.reserve(512);

    Lexer lexer(src, tokens);
    lexer.tokenize();

    memtrack::next_phase();

    Parser parser(tokens);

    auto t0 = high_resolution_clock::now();
    std::vector<std::unique_ptr<Stmt>> statements = parser.parse();
    auto t1 = high_resolution_clock::now();

    auto duration_us = duration_cast<microseconds>(t1 - t0);

    std::cout << "[Parser Execution Time : " << duration_us.count() << " us]\n";

    memtrack::next_phase();

    AstPrinter printer;
    for (const auto &s : statements) {
        std::cout << printer.print(*s) << "\n";
    }

    std::cout << "--------------------------------------------\n";

    memtrack::next_phase();

    Interpreter interp;
    interp.interpret(statements);

    std::cout << "--------------------------------------------\n";

    memtrack::print_stats();

    return true;
}

bool run_prompt() {
    std::string line;
    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, line)) break;
        if (!run(line)) return false;
    }

    return true;
}

bool run_file(char *filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string src = ss.str();

    return run(src);
}

int main(int argc, char **argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [source file]\n";
        return 1;
    }

    if (argc == 2) {
        if (!run_file(argv[1])) return 1;
    } else {
        if (!run_prompt()) return 1;
    }

    return 0;
}