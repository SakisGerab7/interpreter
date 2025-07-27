#include "lexer.hpp"
#include "parser.hpp"

bool run(const std::string &src) {
    // std::cout << "Code:\n";
    // std::cout << src << "\n";

    Lexer lexer(src);
    std::vector<Token> tokens = lexer.tokenize();

    // for (const Token &token : tokens) {
    //     std::cout << token << "\n";
    // }

    Parser parser(tokens);
    std::vector<std::unique_ptr<Stmt>> statements = parser.parse();

    AstPrinter printer;
    for (const auto &s : statements) {
        std::cout << printer.print(*s) << "\n";
    }

    std::cout << "--------------------------------------------\n";

    Interpreter interp;
    interp.interpret(statements);

    // test_tokenizer(tokens);

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