#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

#include "LLVMVisitor.h"
#include "Parser.h"
#include "TypeChecker.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uzycie: " << argv[0] << " <plik_wejsciowy.tc>\n";
        return 1;
    }

    std::string filename = argv[1];

    try {
        std::cout << "[1/3] Parsowanie pliku: " << filename << "...\n";
        std::ifstream file(filename);
        tc::Parser parser(file);
        auto program = parser.parse();

        tc::TypeChecker type_checker;
        program.accept(type_checker);
        auto errors = type_checker.get_errors();

        if (errors.size() > 0) {
            std::cerr << "COMPILATION ERROR" << std::endl;
            for (auto error : errors) {
                std::cerr << "    " << error.get_msg() << std::endl;
            }
            return 1;
        }

        std::cout << "[2/3] Generowanie kodu LLVM IR...\n";
        tc::LLVMVisitor compiler;
        program.accept(compiler);

        std::cout << "[3/3] Zapisywanie do pliku output.ll...\n";
        std::string out_filename = "output.ll";
        std::error_code ec;
        
        llvm::raw_fd_ostream dest(out_filename, ec, llvm::sys::fs::OF_None);
        if (ec) {
            std::cerr << "Blad zapisu do pliku: " << ec.message() << "\n";
            return 1;
        }

        compiler.dump_ir(dest);
        dest.flush();
    } catch (const std::exception& e) {
        std::cerr << "\n[BLAD KOMPILACJI]: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
