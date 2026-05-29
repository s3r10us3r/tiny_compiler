#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

#include "Ast.h"

namespace tc {
    struct VariableInfo {
        llvm::Value* memory_location; // AllocaInst* dla zmiennych, surowy wskaźnik dla 'self'
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

            std::vector<std::unordered_map<std::string, VariableInfo>> scopes;
            std::unordered_map<std::string, VariableInfo> globals;

            CodegenContext() {
                llvm_ctx = std::make_unique<llvm::LLVMContext>();
                module = std::make_unique<llvm::Module>("TinyCompiler", *llvm_ctx);
                builder = std::make_unique<llvm::IRBuilder<>>(*llvm_ctx);
                scopes.push_back({});
            }

            llvm::LLVMContext& get_ctx() { return *llvm_ctx; }
            llvm::AllocaInst* create_variable(const std::string& name, llvm::Type* llvm_type, std::shared_ptr<tc::TypeAst> ast_type) {
                llvm::AllocaInst* alloc = builder->CreateAlloca(llvm_type, nullptr, name);
                scopes.back()[name] = {alloc, ast_type};
                return alloc;
            }

            llvm::GlobalVariable* create_global(const std::string& name, llvm::Type* llvm_type, std::shared_ptr<tc::TypeAst> ast_type) {
                auto* gvar = new llvm::GlobalVariable(
                    *module, llvm_type, false,
                    llvm::GlobalValue::InternalLinkage,
                    llvm::Constant::getNullValue(llvm_type),
                    name);
                globals[name] = {gvar, ast_type};
                return gvar;
            }

        VariableInfo* get_variable(const std::string& name) {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end())
                    return &found->second;
            }
            auto git = globals.find(name);
            if (git != globals.end())
                return &git->second;
            return nullptr;
        }

        void push_scope() {
            scopes.push_back({});
        }

        void pop_scope() {
            scopes.pop_back();
        }
    };
}
