#include "LLVMVisitor.h"
#include "Codegen.h"
#include "Lexer.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <memory>
#include <stdexcept>


void tc::LLVMVisitor::visit(tc::ProgramAst* node) {
    // Pass 1: create all struct LLVM types (so globals and functions can reference them).
    for (auto& stmt : node->statements) {
        if (dynamic_cast<tc::StructDeclStmtAst*>(stmt.get())) {
            stmt->accept(*this); // creates struct type + registers field defs
        }
    }

    // Pass 1.5: create LLVM global variables for all top-level let declarations so
    //           functions compiled in Pass 2 can reference them.
    for (auto& stmt : node->statements) {
        if (auto* vd = dynamic_cast<tc::VarDeclStmtAst*>(stmt.get())) {
            codegen_context.create_global(vd->name, get_llvm_type(vd->type.get()), vd->type);
        }
    }

    // Pass 2: emit all standalone functions and struct methods.
    //         Methods are emitted via StructDeclStmtAst; standalone via FuncDeclStmtAst.
    for (auto& stmt : node->statements) {
        if (dynamic_cast<tc::FuncDeclStmtAst*>(stmt.get())) {
            stmt->accept(*this);
        }
        if (auto* sd = dynamic_cast<tc::StructDeclStmtAst*>(stmt.get())) {
            for (auto& m : sd->methods) {
                m->accept(*this); // emits StructName_methodName(ptr %self, ...)
            }
        }
    }

    // Pass 3: wrap remaining top-level statements in main().
    auto* func_type = llvm::FunctionType::get(codegen_context.builder->getInt32Ty(), false);
    auto* main_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        "main",
        codegen_context.module.get()
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        codegen_context.get_ctx(), "entry", main_func);
    codegen_context.builder->SetInsertPoint(entry);

    for (auto& stmt : node->statements) {
        if (!dynamic_cast<tc::FuncDeclStmtAst*>(stmt.get()) &&
            !dynamic_cast<tc::StructDeclStmtAst*>(stmt.get())) {
            stmt->accept(*this);
        }
    }

    // Terminate main — handle the case where a return/break already ended the block.
    if (!codegen_context.builder->GetInsertBlock()->getTerminator()) {
        codegen_context.builder->CreateRet(
            llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)));
    }
}

// -------------------------------------------------------------------------
// Struct declaration
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::StructDeclStmtAst* node) {
    std::vector<llvm::Type*> field_types;
    for (auto& f : node->fields) {
        field_types.push_back(get_llvm_type(f.type.get()));
    }
    auto* st = llvm::StructType::create(codegen_context.get_ctx(), field_types, node->name);
    struct_types[node->name]     = st;
    struct_field_defs[node->name] = node->fields;
}

// -------------------------------------------------------------------------
// Function declaration
// -------------------------------------------------------------------------

void tc::LLVMVisitor::emit_function(tc::FuncDeclStmtAst* node,
                                    const std::string& llvm_name,
                                    bool has_self_ptr) {
    std::vector<llvm::Type*> param_llvm_types;
    for (auto& p : node->params) {
        if (has_self_ptr && p.name == "self") {
            param_llvm_types.push_back(codegen_context.builder->getPtrTy());
            continue;
        }
        param_llvm_types.push_back(get_llvm_type(p.type.get()));
    }

    llvm::Type* ret_llvm_type = get_llvm_type(node->return_type.get());
    auto* func_type = llvm::FunctionType::get(ret_llvm_type, param_llvm_types, false);
    auto* func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage,
        llvm_name, codegen_context.module.get());

    int pi = 0;
    for (auto& arg : func->args())
        arg.setName(node->params[pi++].name);

    llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(
        codegen_context.get_ctx(), "entry", func);
    codegen_context.builder->SetInsertPoint(entry_bb);

    codegen_context.push_scope();
    int i = 0;
    for (auto& arg : func->args()) {
        if (has_self_ptr && node->params[i].name == "self") {
            // 'self' is already a pointer to the struct — store it directly,
            // no alloca needed.  get_pointer_info("self") will return this ptr.
            codegen_context.scopes.back()["self"] = {&arg, node->params[i].type};
            i++;
            continue;
        }
        auto* alloca = codegen_context.builder->CreateAlloca(
            arg.getType(), nullptr, arg.getName());
        codegen_context.builder->CreateStore(&arg, alloca);
        codegen_context.scopes.back()[node->params[i].name] = {alloca, node->params[i].type};
        i++;
    }

    node->body->accept(*this);

    if (!codegen_context.builder->GetInsertBlock()->getTerminator()) {
        if (ret_llvm_type->isVoidTy())
            codegen_context.builder->CreateRetVoid();
        else
            codegen_context.builder->CreateRet(llvm::Constant::getNullValue(ret_llvm_type));
    }

    codegen_context.pop_scope();
}

void tc::LLVMVisitor::visit(tc::FuncDeclStmtAst* node) {
    emit_function(node, node->name, false);
}

void tc::LLVMVisitor::visit(tc::MethodDeclStmtAst* node) {
    emit_function(node, node->owner_struct + "_" + node->name, true);
}

// -------------------------------------------------------------------------
// Return
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::ReturnStmtAst* node) {
    if (node->expr)
        codegen_context.builder->CreateRet(get_value(node->expr.get()));
    else
        codegen_context.builder->CreateRetVoid();
}

// -------------------------------------------------------------------------
// If / Else
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::IfStmtAst* node) {
    llvm::Function* fn = codegen_context.builder->GetInsertBlock()->getParent();

    auto* then_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "if_then",  fn);
    auto* else_bb = node->else_block
        ? llvm::BasicBlock::Create(codegen_context.get_ctx(), "if_else",  fn)
        : nullptr;
    auto* merge_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "if_merge", fn);

    llvm::Value* cond = get_value(node->condition.get());
    codegen_context.builder->CreateCondBr(cond, then_bb, else_bb ? else_bb : merge_bb);

    // then block
    codegen_context.builder->SetInsertPoint(then_bb);
    node->then_block->accept(*this);
    if (!codegen_context.builder->GetInsertBlock()->getTerminator()) {
        codegen_context.builder->CreateBr(merge_bb);
    }

    // else block (optional)
    if (else_bb) {
        codegen_context.builder->SetInsertPoint(else_bb);
        node->else_block->accept(*this);
        if (!codegen_context.builder->GetInsertBlock()->getTerminator()) {
            codegen_context.builder->CreateBr(merge_bb);
        }
    }

    codegen_context.builder->SetInsertPoint(merge_bb);
}

// -------------------------------------------------------------------------
// While / Break
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::WhileStmtAst* node) {
    llvm::Function* fn = codegen_context.builder->GetInsertBlock()->getParent();

    auto* cond_bb  = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_cond", fn);
    auto* body_bb  = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_body", fn);
    auto* after_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "while_after", fn);

    // Register break target so nested 'break' statements can jump here.
    break_target_stack.push_back(after_bb);

    codegen_context.builder->CreateBr(cond_bb);
    codegen_context.builder->SetInsertPoint(cond_bb);

    llvm::Value* cond_val = get_value(node->condition.get());
    codegen_context.builder->CreateCondBr(cond_val, body_bb, after_bb);

    codegen_context.builder->SetInsertPoint(body_bb);
    node->body->accept(*this);
    // Branch back to condition unless the body already terminated (e.g. via break).
    if (!codegen_context.builder->GetInsertBlock()->getTerminator()) {
        codegen_context.builder->CreateBr(cond_bb);
    }

    break_target_stack.pop_back();
    codegen_context.builder->SetInsertPoint(after_bb);
}

void tc::LLVMVisitor::visit(tc::BreakStmtAst* node) {
    if (break_target_stack.empty())
        report_error("'break' used outside of a loop", node->line);
    codegen_context.builder->CreateBr(break_target_stack.back());
}

// -------------------------------------------------------------------------
// Block
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::BlockAst* node) {
    codegen_context.push_scope();
    for (auto& stmt : node->statements) {
        stmt->accept(*this);
    }
    codegen_context.pop_scope();
}

// -------------------------------------------------------------------------
// Variable declaration
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::VarDeclStmtAst* node) {
    auto* llvm_type = get_llvm_type(node->type.get());
    auto* init_val  = get_value(node->init_expr.get());
    bool is_array   = dynamic_cast<tc::ArrayTypeAst*>(node->type.get()) != nullptr;

    // Top-level declarations were pre-registered as LLVM globals in Pass 1.5.
    // scopes.size() == 1 means we're at the true top level of main (not inside a block).
    if (codegen_context.globals.count(node->name) && codegen_context.scopes.size() == 1) {
        auto* gvar = codegen_context.globals[node->name].memory_location;
        if (!is_array) {
            codegen_context.builder->CreateStore(init_val, gvar);
            return;
        }
        emit_array_init(gvar, llvm_type, init_val, count_array_elements(node->type.get()));
        return;
    }

    auto* alloca = codegen_context.create_variable(node->name, llvm_type, node->type);
    if (!is_array) {
        codegen_context.builder->CreateStore(init_val, alloca);
        return;
    }
    emit_array_init(alloca, llvm_type, init_val, count_array_elements(node->type.get()));
}

int tc::LLVMVisitor::count_array_elements(tc::TypeAst* type) {
    int total = 1;
    tc::TypeAst* cur = type;
    while (auto* arr = dynamic_cast<tc::ArrayTypeAst*>(cur)) {
        total *= arr->size;
        cur = arr->base_type.get();
    }
    return total;
}

void tc::LLVMVisitor::emit_array_init(llvm::Value* alloca, llvm::Type* llvm_type,
                                      llvm::Value* init_val, int total_elements) {
    llvm::Function* fn = codegen_context.builder->GetInsertBlock()->getParent();
    auto* cond_bb  = llvm::BasicBlock::Create(codegen_context.get_ctx(), "init_loop_cond", fn);
    auto* body_bb  = llvm::BasicBlock::Create(codegen_context.get_ctx(), "init_loop_body", fn);
    auto* after_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "after_init",     fn);

    codegen_context.builder->CreateBr(cond_bb);
    codegen_context.builder->SetInsertPoint(cond_bb);

    auto* i_phi = codegen_context.builder->CreatePHI(
        llvm::Type::getInt32Ty(codegen_context.get_ctx()), 2, "i");
    i_phi->addIncoming(
        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)),
        cond_bb->getSinglePredecessor());

    auto* cond = codegen_context.builder->CreateICmpSLT(
        i_phi,
        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, total_elements)),
        "init_check");
    codegen_context.builder->CreateCondBr(cond, body_bb, after_bb);

    codegen_context.builder->SetInsertPoint(body_bb);
    llvm::Value* indices[] = {
        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0)),
        i_phi
    };
    auto* element_ptr = codegen_context.builder->CreateGEP(
        llvm_type, alloca, indices, "init_ptr");
    codegen_context.builder->CreateStore(init_val, element_ptr);

    auto* next_i = codegen_context.builder->CreateAdd(
        i_phi,
        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1)),
        "next_i");
    i_phi->addIncoming(next_i, body_bb);

    codegen_context.builder->CreateBr(cond_bb);
    codegen_context.builder->SetInsertPoint(after_bb);
}

// -------------------------------------------------------------------------
// Assignment
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::AssignmentStmtAst* node) {
    llvm::Value*    val         = get_value(node->init_expr.get());
    tc::PointerInfo target_info = get_pointer_info(node->target.get());
    codegen_context.builder->CreateStore(val, target_info.ptr);
}

// -------------------------------------------------------------------------
// Print
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::PrintStmtAst* node) {
    llvm::Value* val = get_value(node->expr.get());
    if (!val) throw std::runtime_error("Error: could not create print value!");

    llvm::FunctionType* printfType = llvm::FunctionType::get(
        codegen_context.builder->getInt32Ty(),
        codegen_context.builder->getPtrTy(), true);
    llvm::FunctionCallee printfFunc =
        codegen_context.module->getOrInsertFunction("printf", printfType);

    llvm::Value* formatStr = nullptr;
    if (val->getType()->isIntegerTy(32)) {
        formatStr = codegen_context.builder->CreateGlobalString("%d\n");
    } else if (val->getType()->isDoubleTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%f\n");
    } else if (val->getType()->isPointerTy()) {
        formatStr = codegen_context.builder->CreateGlobalString("%s\n");
    } else if (val->getType()->isIntegerTy(1)) {
        val = codegen_context.builder->CreateZExt(
            val, codegen_context.builder->getInt32Ty(), "bool_ext");
        formatStr = codegen_context.builder->CreateGlobalString("%d\n");
    } else {
        report_error("Unsupported print type", node->line);
    }

    codegen_context.builder->CreateCall(printfFunc, {formatStr, val});
}

// -------------------------------------------------------------------------
// Read
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::ReadStmtAst* node) {
    tc::PointerInfo info = get_pointer_info(node->target.get());

    llvm::FunctionType* scanfType = llvm::FunctionType::get(
        codegen_context.builder->getInt32Ty(),
        codegen_context.builder->getPtrTy(), true);
    llvm::FunctionCallee scanfFunc =
        codegen_context.module->getOrInsertFunction("scanf", scanfType);

    if (info.type->isIntegerTy(1)) {
        // Read bool via a temporary int, then truncate.
        auto* tmp = codegen_context.builder->CreateAlloca(
            codegen_context.builder->getInt32Ty(), nullptr, "tmp_bool_read");
        auto* fmt = codegen_context.builder->CreateGlobalString("%d");
        codegen_context.builder->CreateCall(scanfFunc, {fmt, tmp});
        auto* loaded = codegen_context.builder->CreateLoad(
            codegen_context.builder->getInt32Ty(), tmp);
        auto* zero = llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 0));
        auto* bval = codegen_context.builder->CreateICmpNE(loaded, zero, "int_to_bool");
        codegen_context.builder->CreateStore(bval, info.ptr);
        return;
    }

    llvm::Value* fmt = nullptr;
    if      (info.type->isIntegerTy(32)) fmt = codegen_context.builder->CreateGlobalString("%d");
    else if (info.type->isDoubleTy())    fmt = codegen_context.builder->CreateGlobalString("%lf");
    else if (info.type->isPointerTy())   fmt = codegen_context.builder->CreateGlobalString(" %m[^\n]");
    else report_error("Unsupported read type", node->line);

    codegen_context.builder->CreateCall(scanfFunc, {fmt, info.ptr});
}

// -------------------------------------------------------------------------
// Literals
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::IntExprAst* node) {
    last_value = llvm::ConstantInt::get(
        codegen_context.builder->getInt32Ty(), node->val, true);
}

void tc::LLVMVisitor::visit(tc::FloatExprAst* node) {
    last_value = llvm::ConstantFP::get(
        codegen_context.builder->getDoubleTy(), node->val);
}

void tc::LLVMVisitor::visit(tc::BoolExprAst* node) {
    last_value = llvm::ConstantInt::get(
        codegen_context.builder->getInt1Ty(), node->val ? 1 : 0);
}

void tc::LLVMVisitor::visit(tc::StringExprAst* node) {
    last_value = codegen_context.builder->CreateGlobalString(node->val, "str_tmp");
}

// -------------------------------------------------------------------------
// Variable / array / field
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::VariableExprAst* node) {
    auto* var_info = codegen_context.get_variable(node->name);
    if (!var_info) report_error("Unknown variable: " + node->name, node->line);

    last_value = codegen_context.builder->CreateLoad(
        get_llvm_type(var_info->type.get()),
        var_info->memory_location,
        node->name);
}

void tc::LLVMVisitor::visit(tc::ArrayAccessExprAst* node) {
    auto ptr_info = get_pointer_info(node);
    last_value = codegen_context.builder->CreateLoad(
        ptr_info.type, ptr_info.ptr, "array_load");
}

void tc::LLVMVisitor::visit(tc::FieldAccessExprAst* node) {
    auto ptr_info = get_pointer_info(node);
    last_value = codegen_context.builder->CreateLoad(
        ptr_info.type, ptr_info.ptr, node->field_name + "_load");
}

// -------------------------------------------------------------------------
// Unary / Binary
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::UnaryExprAst* node) {
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

void tc::LLVMVisitor::visit(tc::BinaryExprAst* node) {
    // Short-circuit AND / OR handled separately.
    if (node->op != tok_and && node->op != tok_or) {
        auto* left  = get_value(node->left.get());
        auto* right = get_value(node->right.get());
        bool is_float = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

        switch (node->op) {
            case '+': last_value = is_float
                ? codegen_context.builder->CreateFAdd(left, right, "addtmp")
                : codegen_context.builder->CreateAdd(left, right, "addtmp"); break;
            case '-': last_value = is_float
                ? codegen_context.builder->CreateFSub(left, right, "subtmp")
                : codegen_context.builder->CreateSub(left, right, "subtmp"); break;
            case '*': last_value = is_float
                ? codegen_context.builder->CreateFMul(left, right, "multmp")
                : codegen_context.builder->CreateMul(left, right, "multmp"); break;
            case '/': {
                llvm::Function* fn = codegen_context.builder->GetInsertBlock()->getParent();
                auto* div_fail = llvm::BasicBlock::Create(codegen_context.get_ctx(), "div_fail", fn);
                auto* div_ok   = llvm::BasicBlock::Create(codegen_context.get_ctx(), "div_ok",   fn);

                llvm::Value* is_zero;
                if (is_float) {
                    is_zero = codegen_context.builder->CreateFCmpOEQ(
                        right, llvm::ConstantFP::get(codegen_context.builder->getDoubleTy(), 0.0),
                        "is_zero_f");
                } else {
                    is_zero = codegen_context.builder->CreateICmpEQ(
                        right, llvm::ConstantInt::get(codegen_context.builder->getInt32Ty(), 0),
                        "is_zero_i");
                }
                codegen_context.builder->CreateCondBr(is_zero, div_fail, div_ok);

                // Division-by-zero error path
                codegen_context.builder->SetInsertPoint(div_fail);
                {
                    llvm::FunctionType* pt = llvm::FunctionType::get(
                        codegen_context.builder->getInt32Ty(),
                        codegen_context.builder->getPtrTy(), true);
                    llvm::FunctionCallee pf =
                        codegen_context.module->getOrInsertFunction("printf", pt);
                    codegen_context.builder->CreateCall(pf, {
                        codegen_context.builder->CreateGlobalString(
                            "\n[RUNTIME ERROR]: Division by zero!\n")});
                    llvm::FunctionType* et = llvm::FunctionType::get(
                        codegen_context.builder->getVoidTy(),
                        {codegen_context.builder->getInt32Ty()}, false);
                    llvm::FunctionCallee ef =
                        codegen_context.module->getOrInsertFunction("exit", et);
                    codegen_context.builder->CreateCall(ef, {
                        llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1))});
                    codegen_context.builder->CreateUnreachable();
                }

                codegen_context.builder->SetInsertPoint(div_ok);
                last_value = is_float
                    ? codegen_context.builder->CreateFDiv(left, right, "divtmp")
                    : codegen_context.builder->CreateSDiv(left, right, "divtmp");
                break;
            }
            // Relational operators
            case tok_eq:         last_value = is_float
                ? codegen_context.builder->CreateFCmpOEQ(left, right, "eqtmp")
                : codegen_context.builder->CreateICmpEQ(left, right, "eqtmp"); break;
            case tok_neq:        last_value = is_float
                ? codegen_context.builder->CreateFCmpONE(left, right, "neqtmp")
                : codegen_context.builder->CreateICmpNE(left, right, "neqtmp"); break;
            case '<':            last_value = is_float
                ? codegen_context.builder->CreateFCmpOLT(left, right, "lttmp")
                : codegen_context.builder->CreateICmpSLT(left, right, "lttmp"); break;
            case '>':            last_value = is_float
                ? codegen_context.builder->CreateFCmpOGT(left, right, "gttmp")
                : codegen_context.builder->CreateICmpSGT(left, right, "gttmp"); break;
            case tok_less_or_eq: last_value = is_float
                ? codegen_context.builder->CreateFCmpOLE(left, right, "leqtmp")
                : codegen_context.builder->CreateICmpSLE(left, right, "leqtmp"); break;
            case tok_more_or_eq: last_value = is_float
                ? codegen_context.builder->CreateFCmpOGE(left, right, "geqtmp")
                : codegen_context.builder->CreateICmpSGE(left, right, "geqtmp"); break;
            case tok_xor:
                last_value = codegen_context.builder->CreateXor(left, right, "xortmp"); break;
            default:
                throw std::runtime_error("Unknown binary operator: " + std::to_string(node->op));
        }
        return;
    }

    // Short-circuit AND / OR
    llvm::Value* left_val = get_value(node->left.get());
    llvm::BasicBlock* left_bb = codegen_context.builder->GetInsertBlock();
    llvm::Function*   fn      = left_bb->getParent();

    auto* right_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "eval_right", fn);
    auto* merge_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "merge_logic", fn);

    if (node->op == tok_and) {
        codegen_context.builder->CreateCondBr(left_val, right_bb, merge_bb);
    } else {
        codegen_context.builder->CreateCondBr(left_val, merge_bb, right_bb);
    }

    codegen_context.builder->SetInsertPoint(right_bb);
    llvm::Value*      right_val      = get_value(node->right.get());
    llvm::BasicBlock* right_final_bb = codegen_context.builder->GetInsertBlock();
    codegen_context.builder->CreateBr(merge_bb);

    codegen_context.builder->SetInsertPoint(merge_bb);
    auto* phi = codegen_context.builder->CreatePHI(
        codegen_context.builder->getInt1Ty(), 2, "logic_tmp");

    if (node->op == tok_and) {
        phi->addIncoming(codegen_context.builder->getFalse(), left_bb);
        phi->addIncoming(right_val, right_final_bb);
    } else {
        phi->addIncoming(codegen_context.builder->getTrue(), left_bb);
        phi->addIncoming(right_val, right_final_bb);
    }

    last_value = phi;
}

// -------------------------------------------------------------------------
// Function call
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::FuncCallExprAst* node) {
    llvm::Function* callee = codegen_context.module->getFunction(node->name);
    if (!callee) {
        report_error("Undefined function '" + node->name + "'", node->line);
    }

    std::vector<llvm::Value*> args;
    for (auto& arg : node->args) {
        args.push_back(get_value(arg.get()));
    }

    auto* call = codegen_context.builder->CreateCall(
        callee, args,
        callee->getReturnType()->isVoidTy() ? "" : "call_tmp");
    last_value = call;
}

// -------------------------------------------------------------------------
// Method call  obj.method(args)
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::MethodCallExprAst* node) {
    // Get a pointer to the receiver struct so we can pass it as 'self'.
    tc::PointerInfo obj_info = get_pointer_info(node->object.get());

    // Determine struct name from the LLVM struct type.
    auto* st = llvm::dyn_cast<llvm::StructType>(obj_info.type);
    if (!st) {
        report_error("Method call receiver is not a struct type", node->line);
    }
    std::string struct_name = st->getName().str();

    // Look up the mangled function.
    std::string mangled = struct_name + "_" + node->method_name;
    llvm::Function* callee = codegen_context.module->getFunction(mangled);
    if (!callee) {
        report_error("Unknown method '" + node->method_name +
                     "' on struct '" + struct_name + "'", node->line);
    }

    // Build argument list: self pointer first, then user-supplied args.
    std::vector<llvm::Value*> args;
    args.push_back(obj_info.ptr); // self = pointer to the struct on the stack
    for (auto& arg : node->args) {
        args.push_back(get_value(arg.get()));
    }

    auto* call = codegen_context.builder->CreateCall(
        callee, args,
        callee->getReturnType()->isVoidTy() ? "" : "method_tmp");
    last_value = call;
}

// -------------------------------------------------------------------------
// Struct literal
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::StructLiteralExprAst* node) {
    auto st_it = struct_types.find(node->struct_name);
    if (st_it == struct_types.end())
        report_error("Unknown struct type '" + node->struct_name + "'", node->line);

    auto* st = st_it->second;
    auto& field_defs = struct_field_defs.at(node->struct_name);

    // Allocate struct on the stack
    auto* alloca = codegen_context.builder->CreateAlloca(st, nullptr, "struct_tmp");

    // Store each field in the right slot
    for (auto& [fname, fexpr] : node->fields) {
        int idx = -1;
        for (int i = 0; i < (int)field_defs.size(); i++) {
            if (field_defs[i].name == fname) { idx = i; break; }
        }
        if (idx == -1)
            report_error("Unknown field '" + fname + "' in struct '" + node->struct_name + "'", node->line);
        auto* field_ptr = codegen_context.builder->CreateStructGEP(
            st, alloca, (unsigned)idx, fname + "_ptr");
        codegen_context.builder->CreateStore(get_value(fexpr.get()), field_ptr);
    }

    // Load the whole struct as a value
    last_value = codegen_context.builder->CreateLoad(st, alloca, "struct_val");
}

// -------------------------------------------------------------------------
// ExprStmt
// -------------------------------------------------------------------------

void tc::LLVMVisitor::visit(tc::ExprStmtAst* node) {
    node->expr->accept(*this); // result intentionally discarded
}

// -------------------------------------------------------------------------
// IR dump
// -------------------------------------------------------------------------

void tc::LLVMVisitor::dump_ir(llvm::raw_ostream& out) const {
    codegen_context.module->print(out, nullptr);
}

// -------------------------------------------------------------------------
// get_value / get_pointer_info
// -------------------------------------------------------------------------

llvm::Value* tc::LLVMVisitor::get_value(tc::ExprAst* node) {
    node->accept(*this);
    return last_value;
}

tc::PointerInfo tc::LLVMVisitor::get_pointer_info(tc::ExprAst* node) {

    if (auto* var_expr = dynamic_cast<tc::VariableExprAst*>(node)) {
        auto* var_info = codegen_context.get_variable(var_expr->name);
        if (!var_info) report_error("Unknown variable: " + var_expr->name, var_expr->line);
        llvm::Type* t = get_llvm_type(var_info->type.get());
        return {var_info->memory_location, t};
    }

    if (auto* arr_expr = dynamic_cast<tc::ArrayAccessExprAst*>(node)) {
        tc::PointerInfo parent = get_pointer_info(arr_expr->target.get());

        // Bounds check
        uint64_t array_size  = parent.type->getArrayNumElements();
        llvm::Value* idx     = get_value(arr_expr->index.get());
        llvm::Value* size_v  = llvm::ConstantInt::get(
            codegen_context.get_ctx(), llvm::APInt(32, array_size));
        llvm::Value* zero_v  = llvm::ConstantInt::get(
            codegen_context.get_ctx(), llvm::APInt(32, 0));

        llvm::Function* fn = codegen_context.builder->GetInsertBlock()->getParent();
        auto* fail_bb = llvm::BasicBlock::Create(codegen_context.get_ctx(), "bounds_fail", fn);
        auto* ok_bb   = llvm::BasicBlock::Create(codegen_context.get_ctx(), "bounds_ok",   fn);

        auto* is_neg     = codegen_context.builder->CreateICmpSLT(idx, zero_v, "is_neg");
        auto* is_too_big = codegen_context.builder->CreateICmpSGE(idx, size_v, "is_too_big");
        auto* oob        = codegen_context.builder->CreateOr(is_neg, is_too_big, "oob");
        codegen_context.builder->CreateCondBr(oob, fail_bb, ok_bb);

        codegen_context.builder->SetInsertPoint(fail_bb);
        {
            llvm::FunctionType* pt = llvm::FunctionType::get(
                codegen_context.builder->getInt32Ty(),
                codegen_context.builder->getPtrTy(), true);
            llvm::FunctionCallee pf =
                codegen_context.module->getOrInsertFunction("printf", pt);
            codegen_context.builder->CreateCall(pf, {
                codegen_context.builder->CreateGlobalString(
                    "\n[RUNTIME ERROR]: Array index out of bounds!\n")});
            llvm::FunctionType* et = llvm::FunctionType::get(
                codegen_context.builder->getVoidTy(),
                {codegen_context.builder->getInt32Ty()}, false);
            llvm::FunctionCallee ef =
                codegen_context.module->getOrInsertFunction("exit", et);
            codegen_context.builder->CreateCall(ef, {
                llvm::ConstantInt::get(codegen_context.get_ctx(), llvm::APInt(32, 1))});
            codegen_context.builder->CreateUnreachable();
        }

        codegen_context.builder->SetInsertPoint(ok_bb);
        llvm::Value* indices[] = {zero_v, idx};
        auto* elem_ptr = codegen_context.builder->CreateGEP(
            parent.type, parent.ptr, indices, "elem_ptr");
        return {elem_ptr, parent.type->getArrayElementType()};
    }

    if (auto* fa_expr = dynamic_cast<tc::FieldAccessExprAst*>(node)) {
        tc::PointerInfo parent = get_pointer_info(fa_expr->object.get());

        auto* st = llvm::dyn_cast<llvm::StructType>(parent.type);
        if (!st) {
            report_error("Cannot access field of non-struct type", fa_expr->line);
        }

        std::string struct_name = st->getName().str();
        auto& field_defs = struct_field_defs.at(struct_name);

        int idx = -1;
        for (int i = 0; i < (int)field_defs.size(); i++) {
            if (field_defs[i].name == fa_expr->field_name) { idx = i; break; }
        }
        if (idx == -1) {
            report_error("Unknown field '" + fa_expr->field_name + "'", fa_expr->line);
        }

        auto* field_ptr = codegen_context.builder->CreateStructGEP(
            st, parent.ptr, (unsigned)idx, fa_expr->field_name + "_ptr");
        return {field_ptr, st->getElementType((unsigned)idx)};
    }

    report_error("Cannot get memory address for this expression", node->line);
}

// -------------------------------------------------------------------------
// get_llvm_type
// -------------------------------------------------------------------------

llvm::Type* tc::LLVMVisitor::get_llvm_type(tc::TypeAst* type) {
    if (dynamic_cast<tc::IntTypeAst*>(type))
        return llvm::Type::getInt32Ty(codegen_context.get_ctx());
    if (dynamic_cast<tc::FloatTypeAst*>(type))
        return llvm::Type::getDoubleTy(codegen_context.get_ctx());
    if (dynamic_cast<tc::BoolTypeAst*>(type))
        return codegen_context.builder->getInt1Ty();
    if (dynamic_cast<tc::StringTypeAst*>(type))
        return codegen_context.builder->getPtrTy();
    if (dynamic_cast<tc::VoidTypeAst*>(type))
        return llvm::Type::getVoidTy(codegen_context.get_ctx());
    if (auto* arr = dynamic_cast<tc::ArrayTypeAst*>(type))
        return llvm::ArrayType::get(get_llvm_type(arr->base_type.get()), arr->size);
    if (auto* st = dynamic_cast<tc::StructTypeAst*>(type)) {
        auto it = struct_types.find(st->name);
        if (it == struct_types.end())
            throw std::runtime_error("Unknown struct type '" + st->name + "' in codegen");
        return it->second;
    }
    throw std::runtime_error("Unknown type in codegen!");
}

// -------------------------------------------------------------------------
// report_error
// -------------------------------------------------------------------------

[[noreturn]] void tc::LLVMVisitor::report_error(const std::string& message, int line) {
    throw std::runtime_error("[Codegen error at line " + std::to_string(line) + "]: " + message);
}
