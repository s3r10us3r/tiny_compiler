#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

#include "Parser.h"

namespace tc {
    struct VariableInfo {
        llvm::AllocaInst* memory_location;
        std::shared_ptr<TypeAst> type;
    };

    struct PointerInfo {
        llvm::Value* ptr;
        llvm::Type* type;
    };

    class CodegenContext {
        public:
            std::unique_ptr<llvm::LLVMContext> llvm_ctx;
            std::unique_ptr<llvm::IRBuilder<>> builder;
            std::unique_ptr<llvm::Module> module;

            std::unordered_map<std::string, VariableInfo> named_values;

            CodegenContext() {
                llvm_ctx = std::make_unique<llvm::LLVMContext>();
                module = std::make_unique<llvm::Module>("TinyCompiler", *llvm_ctx);
                builder = std::make_unique<llvm::IRBuilder<>>(*llvm_ctx);
            }

            llvm::LLVMContext& get_ctx() { return *llvm_ctx; }
            llvm::AllocaInst* create_variable(const std::string& name, llvm::Type* llvm_type, std::shared_ptr<tc::TypeAst> ast_type) {
                llvm::AllocaInst* alloc = builder->CreateAlloca(llvm_type, nullptr, name);
                named_values[name] = {alloc, ast_type};
                return alloc;
        }

        VariableInfo* get_variable(const std::string& name) {
            auto it = named_values.find(name);
            if (it != named_values.end()) {
                return &it->second;
            }
            return nullptr;
        }
    };
}
