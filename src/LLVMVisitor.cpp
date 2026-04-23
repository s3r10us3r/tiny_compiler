#include "LLVMVisitor.h"
#include "Codegen.h"
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
    auto* left = get_value(node->left.get());
    auto* right = get_value(node->right.get());

    bool is_float = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

    switch (node->op) {
        case '+':
            if (is_float) last_value = codegen_context.builder->CreateFAdd(left, right, "addtmp");
            else          last_value = codegen_context.builder->CreateAdd(left, right, "addtmp");
            break;
        case '-':
            if (is_float) last_value = codegen_context.builder->CreateFSub(left, right, "subtmp");
            else          last_value = codegen_context.builder->CreateSub(left, right, "subtmp");
            break;
        case '*':
            if (is_float) last_value = codegen_context.builder->CreateFMul(left, right, "multmp");
            else          last_value = codegen_context.builder->CreateMul(left, right, "multmp");
            break;
        case '/':
            if (is_float) last_value = codegen_context.builder->CreateFDiv(left, right, "divtmp");
            else          last_value = codegen_context.builder->CreateSDiv(left, right, "divtmp");
            break;
        default:
            throw std::runtime_error(std::string("Blad: Nieznany operator '") + (char)node->op + "'");
    }
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
