#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <system_error>

#include "LLVMVisitor.h"
#include "Parser.h"
#include "TypeChecker.h"
#include "MermaidVisitor.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-m] <input_file.tc>\n";
        std::cerr << "Options:\n  -m    Output Mermaid AST diagram to stdout\n";
        return 1;
    }

    std::string filename;
    bool show_mermaid = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-m") {
            show_mermaid = true;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No input file specified.\n";
        return 1;
    }

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << "\n";
            return 1;
        }

        std::cout << "[1/3] Parsing file: " << filename << "...\n";
        tc::Parser parser(file);
        auto program = parser.parse();

        if (show_mermaid) {
            tc::MermaidVisitor mermaid_visitor;
            program.accept(mermaid_visitor);
            std::cout << "\n--- MERMAID DIAGRAM START ---\n";
            std::cout << mermaid_visitor.get_code();
            std::cout << "--- MERMAID DIAGRAM END ---\n\n";
        }

        std::cout << "[2/3] Type checking...\n";
        tc::TypeChecker type_checker;
        program.accept(type_checker);
        auto errors = type_checker.get_errors();

        if (!errors.empty()) {
            std::cerr << "COMPILATION ERROR\n";
            for (auto& error : errors) {
                std::cerr << "    " << error.get_msg() << std::endl;
            }
            return 1;
        }

        std::cout << "[3/3] Generating LLVM IR...\n";
        tc::LLVMVisitor compiler;
        program.accept(compiler);

        std::string out_filename = "output.ll";
        std::cout << "Saving IR to: " << out_filename << "...\n";
        
        std::error_code ec;
        llvm::raw_fd_ostream dest(out_filename, ec, llvm::sys::fs::OF_None);
        if (ec) {
            std::cerr << "Error writing to file: " << ec.message() << "\n";
            return 1;
        }

        compiler.dump_ir(dest);
        dest.flush();
        
        std::cout << "\nCompilation successful!\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[COMPILATION ERROR]: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
