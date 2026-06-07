#include "common.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ast_printer.hpp"
#include "codegen.hpp"
#include "vm.hpp"
#include "memory.hpp"
#include "serializer.hpp"

#define MEM_TRACKING 0
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

void handle_suspend_signal(int) {
    vm_signal::suspend_requested = 1;
}

int run(std::istream &input, const std::vector<std::string> &args = {}) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::microseconds;

    std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    Lexer lexer(source);

    // memtrack::next_phase();

    Parser parser(lexer);

    std::shared_ptr<std::vector<StmtPtr>> statements = parser.parse();

    // memtrack::next_phase();

    // std::cout << "--------------------------------------------\n";

    AstPrinter printer;
    for (const auto &s : *statements) {
        std::cout << printer.print(*s) << "\n";
    }

    std::cout << "--------------------------------------------\n";

    // memtrack::next_phase();

    Heap heap;

    Codegen gen(&heap);
    auto main_func = gen.compile(statements);
    // gen.disassemble();

    // std::cout << "--------------------------------------------\n";

    // memtrack::next_phase();

    VM vm(args, &heap);

    struct sigaction act = {};
    act.sa_handler = handle_suspend_signal;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);

    Value result = vm.interpret(main_func);

    if (vm_signal::suspend_requested) {
        std::cerr << "VM suspended, creating checkpoint image...\n";

        std::ofstream vm_out("vm_state.json");
        std::ofstream vm_out_bin("vm_state.bin", std::ios::binary);

        heap.collect_garbage(); // perform a GC pass to clean up any unreachable objects before traversal

        JsonSerializer serializer(vm_out);
        serializer.print_vm(vm);

        // BinarySerializer bserializer(vm_out_bin);
        // bserializer.print_vm(vm);
    }

    std::cout << "--------------------------------------------\n";

    // memtrack::print_stats();

    if (result.is_null()) return 0;
    if (result.is_int()) return result.as_int();

    return result.is_truthy() ? 0 : 1;
}

int run_prompt() {
    return run(std::cin);
}

int run_file(char *filename, const std::vector<std::string> &args) {
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Error: could not open file " << filename << "\n";
        return 1;
    }

    // namespace fs = std::filesystem;

    // auto f_time = fs::last_write_time(filename);
    // auto s_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
    //     f_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    // );

    // std::time_t c_time = std::chrono::system_clock::to_time_t(s_time);

    // std::stringstream oss;
    // oss << std::put_time(std::localtime(&c_time), "%Y-%m-%d %H:%M:%S");

    // std::cout << "Last time " << filename << " was modified: " << oss.str() << std::endl;

    return run(file, args);
}

int main(int argc, char **argv) {
    std::srand(std::time(nullptr));

    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::cerr << "Usage: " << argv[0] << " [source file] [args...]\n";
        return 1;
    }

    if (argc == 1) {
        return run_prompt();
    }

    return run_file(argv[1], std::vector<std::string>(argv + 1, argv + argc));
}
