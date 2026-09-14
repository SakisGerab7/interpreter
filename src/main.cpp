#include "common.hpp"
#include "deserializer.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ast_printer.hpp"
#include "codegen.hpp"
#include "vm.hpp"
#include "memory.hpp"
#include "serializer.hpp"
#include <iostream>

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

int run(std::istream &input, const std::vector<std::string> &args = {}, const std::string &snapshot_name = "") {
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

    // AstPrinter printer;
    // for (const auto &s : *statements) {
    //     std::cout << printer.print(*s) << "\n";
    // }

    // std::cout << "--------------------------------------------\n";

    // memtrack::next_phase();

    Heap heap;

    Codegen gen(&heap);
    auto main_func = gen.compile(statements);
    // gen.disassemble();

    // std::cout << "--------------------------------------------\n";

    // memtrack::next_phase();

    auto vm = VM::create(args, &heap);

    struct sigaction act = {};
    act.sa_handler = handle_suspend_signal;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);

    Value result = vm->interpret(main_func);

    if (vm_signal::suspend_requested) {
        std::cerr << "VM suspended, creating checkpoint image...\n";
        heap.collect_garbage(); // perform a GC pass to clean up any unreachable objects before traversal

        std::cerr << "Garbage collected\n";

        std::ofstream vm_out("vm_state.json");
        JsonSerializer serializer(vm_out);
        serializer.print_vm(*vm);
        vm_out.flush();
        vm_out.close();

        std::ofstream vm_out_bin(snapshot_name, std::ios::binary);
        BinarySerializer bserializer(vm_out_bin);
        bserializer.print_vm(*vm);
        vm_out_bin.flush();
        vm_out_bin.close();
    }

    // std::cout << "--------------------------------------------\n";

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

    // Check if the file extension is .dog
    std::string extension = std::filesystem::path(filename).extension().string();
    if (extension != ".dog") {
        std::cerr << "Error: file extension must be .dog\n";
        return 1;
    }

    // For snapshots, replace .dog with .snapshot
    std::string snapshot_name = std::filesystem::path(filename).replace_extension(".snapshot").string();

    return run(file, args, snapshot_name);
}

int run_snapshot(char *snapshot_file, const std::vector<std::string> &args) {
    Heap heap;
    auto vm = VM::load(snapshot_file, &heap, args);

    struct sigaction act = {};
    act.sa_handler = handle_suspend_signal;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);

    Value result = vm->resume();

    if (result.is_null()) return 0;
    if (result.is_int()) return result.as_int();
    return result.is_truthy() ? 0 : 1;
}

int main(int argc, char **argv) {
    std::srand(std::time(nullptr));

    if (argc > 1 && std::string(argv[1]) == "--help") {
        std::cerr << "Usage: " << argv[0] << " [source file] [args...]\n"
                  << "       " << argv[0] << " --snapshot <snapshot file> [args...]\n";
        return 1;
    }

    if (argc == 1) {
        return run_prompt();
    }

    if (std::string(argv[1]) == "--snapshot") {
        if (argc < 3) {
            std::cerr << "Error: missing snapshot file path.\n"
                      << "Usage: " << argv[0] << " --snapshot <snapshot file> [args...]\n";
            return 1;
        }
        return run_snapshot(argv[2], std::vector<std::string>(argv + 3, argv + argc));
    }

    return run_file(argv[1], std::vector<std::string>(argv + 1, argv + argc));
}
