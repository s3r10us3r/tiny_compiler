#include "LLVMVisitor.h"
#include "Parser.h"
#include <llvm/IR/DerivedTypes.h>

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
    codegen_context.builder->CreateStore(value, alloca);
}

void tc::LLVMVisitor::visit(tc::VariableExprAst* node) {
    auto* var_info = codegen_context.get_variable(node->name);

    last_value = codegen_context.builder->CreateLoad(
            get_llvm_type(var_info->type.get()),
            var_info->memory_location,
            node->name
    );
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
    auto* var_info = codegen_context.get_variable(node->name);
    auto* value = get_value(node->init_expr.get());
    codegen_context.builder->CreateStore(value, var_info->memory_location);
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
    throw std::runtime_error("Error: unknown type during code generation!");
}


void tc::LLVMVisitor::visit(tc::PrintStmtAst* node) {
    llvm::Value* val = get_value(node->expr.get());
    if (!val) {
        throw std::runtime_error("Blad: Nie udalo sie wygenerowac wartosci dla print");
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
    } else {
        throw std::runtime_error("Unsupported print type");
    }

    codegen_context.builder->CreateCall(printfFunc, {formatStr, val});
}


void tc::LLVMVisitor::visit(tc::ReadStmtAst* node) {
    auto* var_info = codegen_context.get_variable(node->name);

    llvm::FunctionType* scanfType = llvm::FunctionType::get(
        codegen_context.builder->getInt32Ty(),
        codegen_context.builder->getPtrTy(),
        true
    );
    llvm::FunctionCallee scanfFunc = codegen_context.module->getOrInsertFunction("scanf", scanfType);

    llvm::Type* llvm_type = get_llvm_type(var_info->type.get());
    llvm::Value* formatStr = nullptr;

    if (llvm_type->isIntegerTy(32)) {
        formatStr = codegen_context.builder->CreateGlobalString("%d");
    } else if (llvm_type->isDoubleTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%lf");
    } else {
        throw std::runtime_error("Unsupported read type!");
    }

    codegen_context.builder->CreateCall(scanfFunc, {formatStr, var_info->memory_location});
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

