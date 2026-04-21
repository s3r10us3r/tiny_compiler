cmake_minimum_required(VERSION 3.10)
project(TinyCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. SZUKAMY ZAINSTALOWANEGO LLVM
find_package(LLVM REQUIRED CONFIG)

# --- TUTAJ SĄ ŚCIEŻKI ---
include_directories(${LLVM_INCLUDE_DIRS}) # Ścieżki do nagłówków (.h)
link_directories(${LLVM_LIBRARY_DIRS})    # <--- TEGO BRAKOWAŁO! Ścieżki do bibliotek (.a / .so)
# ------------------------

add_definitions(${LLVM_DEFINITIONS})

# 2. BUDUJEMY NASZ PROGRAM
add_executable(compiler 
    src/main.cpp 
    src/Lexer.cpp 
    src/Parser.cpp 
    src/LLVMVisitor.cpp
    src/TypeChecker.cpp
    # src/AST.cpp (jeśli masz)
)

target_link_libraries(compiler LLVM)
