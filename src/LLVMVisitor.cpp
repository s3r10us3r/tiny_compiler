#include "LLVMVisitor.h"
#include "Codegen.h"
#include "Lexer.h"
#include "Parser.h"
#include <llvm/IR/DerivedTypes.h>
#include <memory>

void tc::LLVMVisitor::visit(tc::ProgramAst* node) {
    auto func_type = llvm::FunctionType::get(codegen_context.builder->getInt32Ty(), false);
    auto main_func = llvm::Function::Create(
            func_type,
            llvm::Function::ExternalLinkage, // funkcja dostępna dla wszystkich
            "main",
            codegen_context.module.get()
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*codegen_context.llvm_ctx, "entry", main_func);
    codegen_context.builder->SetInsertPoint(entry);

    for (auto &stmt : node->statements) {
        stmt->accept(*this);
    }

    codegen_context.builder->CreateRet(
        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0))
    );
}

void tc::LLVMVisitor::visit(tc::VarDeclStmtAst* node) {
    auto llvm_type = get_llvm_type(node->type.get());
    auto alloca = codegen_context.create_variable(node->name, llvm_type, node->type);

    auto value = get_value(node->init_expr.get());
    auto arr_type = std::dynamic_pointer_cast<tc::ArrayTypeAst>(node->type);

    if (!arr_type) {
        codegen_context.builder->CreateStore(value, alloca);
    }
    else {
        // generujemy pętle, która wypełni tablicę wartością expression po prawej
        int total_elements = 1;
        tc::TypeAst* current = node->type.get();
        while (auto arr = dynamic_cast<tc::ArrayTypeAst*>(current)) {
            total_elements *= arr->size;
            current = arr->base_type.get();
        }

        // TODO: zamienić to w AST na pętle kiedy zostanie dodana
        // wgl można to w całości zamienić generacją kodu w preprocesorze jeżeli przypisanie będzie sprawdzał najpierw type checker
        // struktura pętli
        llvm::Function* parent_func = codegen_context.builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "init_loop_cond", parent_func);
        llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "init_loop_body", parent_func);
        llvm::BasicBlock* after_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "after_init", parent_func);

        //skok do warunku
        codegen_context.builder->CreateBr(cond_bb);
        codegen_context.builder->SetInsertPoint(cond_bb);

        //phi node zachowuje SSA i pozwala n "wybranie" wartości w zależności od bloku z którego przyszliśmy
        auto* i_phi = codegen_context.builder->CreatePHI(llvm::Type::getInt32Ty(codegen_context.get_ctx()), 2, "i");
        i_phi->addIncoming(llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)), codegen_context.builder->GetInsertBlock()->getSinglePredecessor());

        //warunek i < liczby elementów
        auto* cond = codegen_context.builder->CreateICmpSLT(i_phi, llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, total_elements)), "init_check");
        codegen_context.builder->CreateCondBr(cond, body_bb, after_bb);
        codegen_context.builder->SetInsertPoint(body_bb);

        // obliczamy adres komórki
        llvm::Value* indices[] = {  llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)), i_phi };
        auto* element_ptr = codegen_context.builder->CreateGEP(llvm_type, alloca, indices, "init_ptr");
        codegen_context.builder->CreateStore(value, element_ptr);

        auto* next_i = codegen_context.builder->CreateAdd(i_phi, llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1)), "next_i");
        i_phi->addIncoming(next_i, body_bb);

        codegen_context.builder->CreateBr(cond_bb);
        codegen_context.builder->SetInsertPoint(after_bb);
    }
}

void tc::LLVMVisitor::visit(tc::BoolExprAst* node) {
    last_value = llvm::ConstantInt::get(
        codegen_context.builder->getInt1Ty(), 
        node->val ? 1 : 0
    );
}

void tc::LLVMVisitor::visit(tc::BlockAst* node) {
    codegen_context.push_scope();
    for (int i = 0; i < node->statements.size(); i++) {
        node->statements[i]->accept(*this);
    }
    codegen_context.pop_scope();
}

void tc::LLVMVisitor::visit(tc::WhileStmtAst* node) {
    llvm::Function* parent_func = codegen_context.builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* cond_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_cond", parent_func);
    llvm::BasicBlock* body_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_body", parent_func);
    llvm::BasicBlock* after_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_after", parent_func);

    codegen_context.builder->CreateBr(cond_bb);
    codegen_context.builder->SetInsertPoint(cond_bb);

    llvm::Value* cond_val = get_value(node->condition.get());
    codegen_context.builder->CreateCondBr(cond_val, body_bb, after_bb);

    codegen_context.builder->SetInsertPoint(body_bb);
    node->body->accept(*this);
    
    codegen_context.builder->CreateBr(cond_bb);
    codegen_context.builder->SetInsertPoint(after_bb);
}

void tc::LLVMVisitor::visit(tc::VariableExprAst* node) {
    auto* var_info = codegen_context.get_variable(node->name);

    last_value = codegen_context.builder->CreateLoad(
            get_llvm_type(var_info->type.get()),
            var_info->memory_location,
            node->name
    );
}

void tc::LLVMVisitor::visit(StringExprAst* node) {
    last_value = codegen_context.builder->CreateGlobalString(node->val, "str_tmp");
}

void tc::LLVMVisitor::visit(UnaryExprAst* node) {
    llvm::Value* val = get_value(node->operand.get());
    if (node->op == tok_not) {
        last_value = codegen_context.builder->CreateNot(val, "not_tmp");
        return;
    }
    if (val->getType()->isDoubleTy()) {
        last_value = codegen_context.builder->CreateFNeg(val, "fneg_tmp");
    } else {
        last_value = codegen_context.builder->CreateNeg(val, "neg_tmp");
    }
}

void tc::LLVMVisitor::visit(tc::AssignmentStmtAst* node) {
    llvm::Value* val = get_value(node->init_expr.get());

    tc::PointerInfo target_info = get_pointer_info(node->target.get());
    codegen_context.builder->CreateStore(val, target_info.ptr);
}


void tc::LLVMVisitor::visit(ArrayAccessExprAst* node) {
    auto pointer_info = get_pointer_info(node);
    last_value = codegen_context.builder->CreateLoad(pointer_info.type, pointer_info.ptr, "array_load");
}

void tc::LLVMVisitor::dump_ir(llvm::raw_ostream& out) const {
    codegen_context.module->print(out, nullptr);
}


llvm::Value* tc::LLVMVisitor::get_value(tc::ExprAst* node) {
    node->accept(*this);
    return last_value;
}

llvm::Type* tc::LLVMVisitor::get_llvm_type(tc::TypeAst* type) {
    if (dynamic_cast<tc::IntTypeAst*>(type)) {
        return llvm::Type::getInt32Ty(codegen_context.get_ctx());
    } 
    else if (dynamic_cast<tc::FloatTypeAst*>(type)) {
        return llvm::Type::getDoubleTy(codegen_context.get_ctx());
    } 
    else if (auto arr_type = dynamic_cast<tc::ArrayTypeAst*>(type)) {
        auto* element_type = get_llvm_type(arr_type->base_type.get());
        return llvm::ArrayType::get(element_type, arr_type->size);
    }
    else if (dynamic_cast<tc::StringTypeAst*>(type)) {
        return codegen_context.builder->getPtrTy();
    }
    else if (dynamic_cast<tc::BoolTypeAst*>(type)) {
        return codegen_context.builder->getInt1Ty();
    }
    throw std::runtime_error("Error: unknown type during code generation!");
}


void tc::LLVMVisitor::visit(tc::PrintStmtAst* node) {
    llvm::Value* val = get_value(node->expr.get());
    if (!val) {
        throw std::runtime_error("Error: could not create print value!");
    }

    // sygnatura funkcji printf
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        codegen_context.builder->getInt32Ty(),
        codegen_context.builder->getPtrTy(), 
        true
    );

    llvm::FunctionCallee printfFunc = codegen_context.module->getOrInsertFunction("printf", printfType);

    llvm::Value* formatStr = nullptr;
    if (val->getType()->isIntegerTy(32)) {
        formatStr = codegen_context.builder->CreateGlobalString("%d\n"); 
    } else if (val->getType()->isDoubleTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%f\n");
    } else if (val->getType()->isPointerTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%s\n");
    } else if (val->getType()->isIntegerTy(1)) {
        val = codegen_context.builder->CreateZExt(val, codegen_context.builder->getInt32Ty(), "bool_ext");
        formatStr = codegen_context.builder->CreateGlobalString("%d\n");
    } else {
        report_error("Unsupported print type", node->line);
    }

    codegen_context.builder->CreateCall(printfFunc, {formatStr, val});
}


void tc::LLVMVisitor::visit(tc::ReadStmtAst* node) {
    tc::PointerInfo info = get_pointer_info(node->target.get());

    llvm::FunctionType* scanfType = llvm::FunctionType::get(
        codegen_context.builder->getInt32Ty(),
        codegen_context.builder->getPtrTy(),
        true
    );
    llvm::FunctionCallee scanfFunc = codegen_context.module->getOrInsertFunction("scanf", scanfType);


    if (info.type->isIntegerTy(1)) {
        llvm::Value* formatStr = codegen_context.builder->CreateGlobalString("%d");
        llvm::Value* temp_int_ptr = codegen_context.builder->CreateAlloca(codegen_context.builder->getInt32Ty(), nullptr, "tmp_bool_read");
        codegen_context.builder->CreateCall(scanfFunc, {formatStr, temp_int_ptr});
        llvm::Value* loaded_int = codegen_context.builder->CreateLoad(codegen_context.builder->getInt32Ty(), temp_int_ptr);
        llvm::Value* zero = llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0));
        llvm::Value* bool_val = codegen_context.builder->CreateICmpNE(loaded_int, zero, "int_to_bool");
        codegen_context.builder->CreateStore(bool_val, info.ptr);
        return; 
    }

    llvm::Value* formatStr = nullptr;
    if (info.type->isIntegerTy(32)) {
        formatStr = codegen_context.builder->CreateGlobalString("%d");
    } else if (info.type->isDoubleTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%lf");
    } else if (info.type->isPointerTy()) {
        formatStr = codegen_context.builder->CreateGlobalString(" %m[^\n]");
    }
    else {
        report_error("Unsupported read type", node->line);
    }

    codegen_context.builder->CreateCall(scanfFunc, {formatStr, info.ptr});
}

void tc::LLVMVisitor::visit(tc::IntExprAst* node) {
    last_value = llvm::ConstantInt::get(
        codegen_context.builder->getInt32Ty(), 
        node->val, 
        true
    );
}

void tc::LLVMVisitor::visit(tc::FloatExprAst* node) {
    last_value = llvm::ConstantFP::get(
        codegen_context.builder->getDoubleTy(), 
        node->val
    );
}

void tc::LLVMVisitor::visit(tc::BinaryExprAst* node) {
    // +, -, / , *
    if (node->op != tok_and && node->op != tok_or) {
        auto* left = get_value(node->left.get());
        auto* right = get_value(node->right.get());
        bool is_float = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

        switch (node->op) {
            // Matematyka
            case '+': last_value = is_float ? codegen_context.builder->CreateFAdd(left, right, "addtmp") : codegen_context.builder->CreateAdd(left, right, "addtmp"); break;
            case '-': last_value = is_float ? codegen_context.builder->CreateFSub(left, right, "subtmp") : codegen_context.builder->CreateSub(left, right, "subtmp"); break;
            case '*': last_value = is_float ? codegen_context.builder->CreateFMul(left, right, "multmp") : codegen_context.builder->CreateMul(left, right, "multmp"); break;
            case '/': {
                llvm::Function* parent_func = codegen_context.builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* div_fail_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "div_fail", parent_func);
                llvm::BasicBlock* div_ok_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "div_ok", parent_func);

                // Sprawdzamy czy dzielnik to 0 
                llvm::Value* is_zero = nullptr;
                if (is_float) {
                    llvm::Value* zero_float = llvm::ConstantFP::get(codegen_context.builder->getDoubleTy(), 0.0);
                    is_zero = codegen_context.builder->CreateFCmpOEQ(right, zero_float, "is_zero_float");
                } else {
                    llvm::Value* zero_int = llvm::ConstantInt::get(codegen_context.builder->getInt32Ty(), 0);
                    is_zero = codegen_context.builder->CreateICmpEQ(right, zero_int, "is_zero_int");
                }

                // Skok warunkowy
                codegen_context.builder->CreateCondBr(is_zero, div_fail_bb, div_ok_bb);

                // crash
                codegen_context.builder->SetInsertPoint(div_fail_bb);
                
                llvm::FunctionType* printfType = llvm::FunctionType::get(codegen_context.builder->getInt32Ty(), codegen_context.builder->getPtrTy(), true);
                llvm::FunctionCallee printfFunc = codegen_context.module->getOrInsertFunction("printf", printfType);
                llvm::Value* errorStr = codegen_context.builder->CreateGlobalString("\n[RUNTIME ERROR]: Division by zero!\n");
                codegen_context.builder->CreateCall(printfFunc, {errorStr});

                llvm::FunctionType* exitType = llvm::FunctionType::get(codegen_context.builder->getVoidTy(), {codegen_context.builder->getInt32Ty()}, false);
                llvm::FunctionCallee exitFunc = codegen_context.module->getOrInsertFunction("exit", exitType);
                codegen_context.builder->CreateCall(exitFunc, {llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1))});
                codegen_context.builder->CreateUnreachable();

                // happy path
                codegen_context.builder->SetInsertPoint(div_ok_bb);
                last_value = is_float ? codegen_context.builder->CreateFDiv(left, right, "divtmp") : codegen_context.builder->CreateSDiv(left, right, "divtmp");
                break;
            }
            // Relacje
            case tok_eq:         last_value = is_float ? codegen_context.builder->CreateFCmpOEQ(left, right, "eqtmp") : codegen_context.builder->CreateICmpEQ(left, right, "eqtmp"); break;
            case tok_neq:        last_value = is_float ? codegen_context.builder->CreateFCmpONE(left, right, "neqtmp") : codegen_context.builder->CreateICmpNE(left, right, "neqtmp"); break;
            case '<':            last_value = is_float ? codegen_context.builder->CreateFCmpOLT(left, right, "lttmp") : codegen_context.builder->CreateICmpSLT(left, right, "lttmp"); break;
            case '>':            last_value = is_float ? codegen_context.builder->CreateFCmpOGT(left, right, "gttmp") : codegen_context.builder->CreateICmpSGT(left, right, "gttmp"); break;
            case tok_less_or_eq: last_value = is_float ? codegen_context.builder->CreateFCmpOLE(left, right, "leqtmp") : codegen_context.builder->CreateICmpSLE(left, right, "leqtmp"); break;
            case tok_xor: last_value = codegen_context.builder->CreateXor(left, right, "xortmp"); break;
            case tok_more_or_eq: last_value = is_float ? codegen_context.builder->CreateFCmpOGE(left, right, "geqtmp") : codegen_context.builder->CreateICmpSGE(left, right, "geqtmp"); break;
            
            default: throw std::runtime_error(std::string("Blad: Nieznany operator '") + std::to_string(node->op) + "'");
        }
        return;
    }

    // AND / OR
    llvm::Value* left_val = get_value(node->left.get());
    if (!left_val) return;

    llvm::BasicBlock* current_bb = codegen_context.builder->GetInsertBlock();
    llvm::Function* parent_func = current_bb->getParent();

    llvm::BasicBlock* right_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "eval_right", parent_func);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "merge_logic", parent_func);

    // SKOK WARUNKOWY
    if (node->op == tok_and) {
        codegen_context.builder->CreateCondBr(left_val, right_bb, merge_bb);
    } else { 
        codegen_context.builder->CreateCondBr(left_val, merge_bb, right_bb);
    }

    codegen_context.builder->SetInsertPoint(right_bb);
    llvm::Value* right_val = get_value(node->right.get());
    llvm::BasicBlock* right_final_bb = codegen_context.builder->GetInsertBlock();
    
    codegen_context.builder->CreateBr(merge_bb);

    codegen_context.builder->SetInsertPoint(merge_bb);
    llvm::PHINode* phi = codegen_context.builder->CreatePHI(codegen_context.builder->getInt1Ty(), 2, "logic_tmp");

    if (node->op == tok_and) {
        // Ścieżka skrócona
        phi->addIncoming(codegen_context.builder->getFalse(), current_bb);
        // Ścieżka długa
        phi->addIncoming(right_val, right_final_bb);
    } else { 
        // Ścieżka skrócona
        phi->addIncoming(codegen_context.builder->getTrue(), current_bb);
        // Ścieżka długa
        phi->addIncoming(right_val, right_final_bb);
    }

    last_value = phi;
}


tc::PointerInfo tc::LLVMVisitor::get_pointer_info(tc::ExprAst* node) {
    if (auto var_expr = dynamic_cast<tc::VariableExprAst*>(node)) {
        auto* var_info = codegen_context.get_variable(var_expr->name);
        if (!var_info) {
            report_error("Unknown variable: " + var_expr->name, var_expr->line);
        }

        llvm::Type* llvm_target_type = get_llvm_type(var_info->type.get());
        return tc::PointerInfo{var_info->memory_location, llvm_target_type};
        
    } else if (auto arr_expr = dynamic_cast<tc::ArrayAccessExprAst*>(node)) {
        tc::PointerInfo parent_info = get_pointer_info(arr_expr->target.get());
        llvm::Value* index_val = get_value(arr_expr->index.get());

        uint64_t array_size = parent_info.type->getArrayNumElements();
        llvm::Value* size_val = llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, array_size));
        llvm::Value* zero_val = llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0));

        llvm::Function* parent_func = codegen_context.builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* bounds_fail_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "bounds_fail", parent_func);
        llvm::BasicBlock* bounds_ok_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "bounds_ok", parent_func);

        llvm::Value* is_neg = codegen_context.builder->CreateICmpSLT(index_val, zero_val, "is_neg");
        llvm::Value* is_too_big = codegen_context.builder->CreateICmpSGE(index_val, size_val, "is_too_big");
        llvm::Value* out_of_bounds = codegen_context.builder->CreateOr(is_neg, is_too_big, "out_of_bounds");

        codegen_context.builder->CreateCondBr(out_of_bounds, bounds_fail_bb, bounds_ok_bb);

        codegen_context.builder->SetInsertPoint(bounds_fail_bb);
        
        llvm::FunctionType* printfType = llvm::FunctionType::get(codegen_context.builder->getInt32Ty(), codegen_context.builder->getPtrTy(), true);
        llvm::FunctionCallee printfFunc = codegen_context.module->getOrInsertFunction("printf", printfType);
        llvm::Value* errorStr = codegen_context.builder->CreateGlobalString("\n[RUNTIME ERROR]: Array index out of bounds!\n");
        codegen_context.builder->CreateCall(printfFunc, {errorStr});

        llvm::FunctionType* exitType = llvm::FunctionType::get(codegen_context.builder->getVoidTy(), {codegen_context.builder->getInt32Ty()}, false);
        llvm::FunctionCallee exitFunc = codegen_context.module->getOrInsertFunction("exit", exitType);
        codegen_context.builder->CreateCall(exitFunc, {llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1))});

        codegen_context.builder->CreateUnreachable();

        codegen_context.builder->SetInsertPoint(bounds_ok_bb);

        llvm::Value* indices[] = {
            llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)),
            index_val
        };

        llvm::Value* element_ptr = codegen_context.builder->CreateGEP(
            parent_info.type, parent_info.ptr, indices, "element_ptr"
        );
        
        llvm::Type* element_type = parent_info.type->getArrayElementType();

        return tc::PointerInfo{element_ptr, element_type};
    }
    
    report_error("Cannot get memory pointer for this expression!", node->line);
}


[[noreturn]] void tc::LLVMVisitor::report_error(const std::string& message, int line) {
    std::string full_message = "[Compilation error at: " + std::to_string(line)
        + "]: "+ message;
    throw std::runtime_error(full_message);
}
