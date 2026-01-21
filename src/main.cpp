#include "common.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ast_printer.hpp"
#include "codegen.hpp"
#include "vm.hpp"

#define MEM_TRACKING 1
#if MEM_TRACKING

namespace memtrack {

struct MemStats {
    size_t allocations, max_allocations;
    size_t num_bytes, max_num_bytes;
};

static MemStats stats[5];

static const char *phase_names[] = {
    "lexing",
    "parsing",
    "printing",
    "code generation",
    "execution"
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

int run(std::istream &input) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;

    std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    Lexer lexer(source);

    memtrack::next_phase();

    Parser parser(lexer);

    auto t0 = high_resolution_clock::now();
    std::shared_ptr<std::vector<StmtPtr>> statements = parser.parse();
    auto t1 = high_resolution_clock::now();

    auto duration_us = duration_cast<microseconds>(t1 - t0);

    std::cout << "[Parser Execution Time : " << duration_us.count() << " us]\n";

    memtrack::next_phase();

    std::cout << "--------------------------------------------\n";
    std::cout << "Parsed " << statements->size() << " statements:\n";
    
    AstPrinter printer;
    for (const auto &s : *statements) {
        std::cout << printer.print(*s) << "\n";
    }

    std::cout << "--------------------------------------------\n";
    
    memtrack::next_phase();
    
    Codegen gen;
    auto main_func = gen.compile(statements);
    gen.disassemble();
    
    std::cout << "--------------------------------------------\n";

    memtrack::next_phase();

    VM vm;
    Value result = vm.interpret(main_func);

    std::cout << "--------------------------------------------\n";

    memtrack::print_stats();

    if (result.is_null()) return 0;
    if (result.is_int()) return result.as_int();

    return result.is_truthy() ? 0 : 1;
}

int run_prompt() {
    return run(std::cin);
}

int run_file(char *filename) {
    std::ifstream file(filename);   

    if (!file) {
        std::cerr << "Error: could not open file " << filename << "\n";
        return 1;
    }

    return run(file);
}

int main(int argc, char **argv) {
    std::srand(std::time(nullptr));

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [source file]\n";
        return 1;
    }

    if (argc == 2) {
        return run_file(argv[1]);
    } else {
        return run_prompt();
    }

    return 0;
}