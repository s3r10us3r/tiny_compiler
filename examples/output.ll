; ModuleID = 'TinyCompiler'
source_filename = "TinyCompiler"

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@2 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@4 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@5 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@6 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@7 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@8 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@9 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@10 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@11 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@12 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@13 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@14 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@15 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@16 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
entry:
  %arr = alloca [5 x i32], align 4
  br label %init_loop_cond

init_loop_cond:                                   ; preds = %init_loop_body, %entry
  %i = phi i32 [ 0, %entry ], [ %next_i, %init_loop_body ]
  %init_check = icmp slt i32 %i, 5
  br i1 %init_check, label %init_loop_body, label %after_init

init_loop_body:                                   ; preds = %init_loop_cond
  %init_ptr = getelementptr [5 x i32], ptr %arr, i32 0, i32 %i
  store i32 1, ptr %init_ptr, align 4
  %next_i = add i32 %i, 1
  br label %init_loop_cond

after_init:                                       ; preds = %init_loop_cond
  %element_ptr = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  store i32 10, ptr %element_ptr, align 4
  %element_ptr1 = getelementptr [5 x i32], ptr %arr, i32 0, i32 4
  store i32 50, ptr %element_ptr1, align 4
  %element_ptr2 = getelementptr [5 x i32], ptr %arr, i32 0, i32 1
  %array_load = load i32, ptr %element_ptr2, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @0, i32 %array_load)
  %element_ptr3 = getelementptr [5 x i32], ptr %arr, i32 0, i32 2
  %array_load4 = load i32, ptr %element_ptr3, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @1, i32 %array_load4)
  %element_ptr5 = getelementptr [5 x i32], ptr %arr, i32 0, i32 3
  %array_load6 = load i32, ptr %element_ptr5, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @2, i32 %array_load6)
  %element_ptr7 = getelementptr [5 x i32], ptr %arr, i32 0, i32 4
  %array_load8 = load i32, ptr %element_ptr7, align 4
  %3 = call i32 (ptr, ...) @printf(ptr @3, i32 %array_load8)
  %element_ptr9 = getelementptr [5 x i32], ptr %arr, i32 0, i32 5
  %array_load10 = load i32, ptr %element_ptr9, align 4
  %4 = call i32 (ptr, ...) @printf(ptr @4, i32 %array_load10)
  %sum = alloca i32, align 4
  %element_ptr11 = getelementptr [5 x i32], ptr %arr, i32 0, i32 0
  %array_load12 = load i32, ptr %element_ptr11, align 4
  %element_ptr13 = getelementptr [5 x i32], ptr %arr, i32 0, i32 4
  %array_load14 = load i32, ptr %element_ptr13, align 4
  %addtmp = add i32 %array_load12, %array_load14
  store i32 %addtmp, ptr %sum, align 4
  %sum15 = load i32, ptr %sum, align 4
  %5 = call i32 (ptr, ...) @printf(ptr @5, i32 %sum15)
  %mat = alloca [2 x [3 x i32]], align 4
  br label %init_loop_cond16

init_loop_cond16:                                 ; preds = %init_loop_body17, %after_init
  %i19 = phi i32 [ 0, %after_init ], [ %next_i22, %init_loop_body17 ]
  %init_check20 = icmp slt i32 %i19, 6
  br i1 %init_check20, label %init_loop_body17, label %after_init18

init_loop_body17:                                 ; preds = %init_loop_cond16
  %init_ptr21 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 %i19
  store i32 0, ptr %init_ptr21, align 4
  %next_i22 = add i32 %i19, 1
  br label %init_loop_cond16

after_init18:                                     ; preds = %init_loop_cond16
  %element_ptr23 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr24 = getelementptr [3 x i32], ptr %element_ptr23, i32 0, i32 0
  store i32 1, ptr %element_ptr24, align 4
  %element_ptr25 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr26 = getelementptr [3 x i32], ptr %element_ptr25, i32 0, i32 1
  store i32 2, ptr %element_ptr26, align 4
  %element_ptr27 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr28 = getelementptr [3 x i32], ptr %element_ptr27, i32 0, i32 2
  store i32 3, ptr %element_ptr28, align 4
  %element_ptr29 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 1
  %element_ptr30 = getelementptr [3 x i32], ptr %element_ptr29, i32 0, i32 0
  store i32 7, ptr %element_ptr30, align 4
  %element_ptr31 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 1
  %element_ptr32 = getelementptr [3 x i32], ptr %element_ptr31, i32 0, i32 1
  store i32 8, ptr %element_ptr32, align 4
  %element_ptr33 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 1
  %element_ptr34 = getelementptr [3 x i32], ptr %element_ptr33, i32 0, i32 2
  store i32 9, ptr %element_ptr34, align 4
  %element_ptr35 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr36 = getelementptr [3 x i32], ptr %element_ptr35, i32 0, i32 0
  %array_load37 = load i32, ptr %element_ptr36, align 4
  %6 = call i32 (ptr, ...) @printf(ptr @6, i32 %array_load37)
  %element_ptr38 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr39 = getelementptr [3 x i32], ptr %element_ptr38, i32 0, i32 1
  %array_load40 = load i32, ptr %element_ptr39, align 4
  %7 = call i32 (ptr, ...) @printf(ptr @7, i32 %array_load40)
  %element_ptr41 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 1
  %element_ptr42 = getelementptr [3 x i32], ptr %element_ptr41, i32 0, i32 0
  %array_load43 = load i32, ptr %element_ptr42, align 4
  %8 = call i32 (ptr, ...) @printf(ptr @8, i32 %array_load43)
  %f_arr = alloca [3 x double], align 8
  br label %init_loop_cond44

init_loop_cond44:                                 ; preds = %init_loop_body45, %after_init18
  %i47 = phi i32 [ 0, %after_init18 ], [ %next_i50, %init_loop_body45 ]
  %init_check48 = icmp slt i32 %i47, 3
  br i1 %init_check48, label %init_loop_body45, label %after_init46

init_loop_body45:                                 ; preds = %init_loop_cond44
  %init_ptr49 = getelementptr [3 x double], ptr %f_arr, i32 0, i32 %i47
  store double 1.100000e+00, ptr %init_ptr49, align 8
  %next_i50 = add i32 %i47, 1
  br label %init_loop_cond44

after_init46:                                     ; preds = %init_loop_cond44
  %element_ptr51 = getelementptr [3 x double], ptr %f_arr, i32 0, i32 1
  store double 4.400000e+00, ptr %element_ptr51, align 8
  %element_ptr52 = getelementptr [3 x double], ptr %f_arr, i32 0, i32 0
  %array_load53 = load double, ptr %element_ptr52, align 8
  %9 = call i32 (ptr, ...) @printf(ptr @9, double %array_load53)
  %element_ptr54 = getelementptr [3 x double], ptr %f_arr, i32 0, i32 1
  %array_load55 = load double, ptr %element_ptr54, align 8
  %10 = call i32 (ptr, ...) @printf(ptr @10, double %array_load55)
  %element_ptr56 = getelementptr [3 x double], ptr %f_arr, i32 0, i32 2
  %array_load57 = load double, ptr %element_ptr56, align 8
  %11 = call i32 (ptr, ...) @printf(ptr @11, double %array_load57)
  %12 = call i32 (ptr, ...) @printf(ptr @12, i32 999999)
  %element_ptr58 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr59 = getelementptr [3 x i32], ptr %element_ptr58, i32 0, i32 0
  %13 = call i32 (ptr, ...) @scanf(ptr @13, ptr %element_ptr59)
  %element_ptr60 = getelementptr [2 x [3 x i32]], ptr %mat, i32 0, i32 0
  %element_ptr61 = getelementptr [3 x i32], ptr %element_ptr60, i32 0, i32 0
  %array_load62 = load i32, ptr %element_ptr61, align 4
  %14 = call i32 (ptr, ...) @printf(ptr @14, i32 %array_load62)
  %arr2 = alloca [5 x i32], align 4
  br label %init_loop_cond63

init_loop_cond63:                                 ; preds = %init_loop_body64, %after_init46
  %i66 = phi i32 [ 0, %after_init46 ], [ %next_i69, %init_loop_body64 ]
  %init_check67 = icmp slt i32 %i66, 5
  br i1 %init_check67, label %init_loop_body64, label %after_init65

init_loop_body64:                                 ; preds = %init_loop_cond63
  %init_ptr68 = getelementptr [5 x i32], ptr %arr2, i32 0, i32 %i66
  store i32 1, ptr %init_ptr68, align 4
  %next_i69 = add i32 %i66, 1
  br label %init_loop_cond63

after_init65:                                     ; preds = %init_loop_cond63
  %mat2 = alloca [6 x [5 x i32]], align 4
  br label %init_loop_cond70

init_loop_cond70:                                 ; preds = %init_loop_body71, %after_init65
  %i73 = phi i32 [ 0, %after_init65 ], [ %next_i76, %init_loop_body71 ]
  %init_check74 = icmp slt i32 %i73, 30
  br i1 %init_check74, label %init_loop_body71, label %after_init72

init_loop_body71:                                 ; preds = %init_loop_cond70
  %init_ptr75 = getelementptr [6 x [5 x i32]], ptr %mat2, i32 0, i32 %i73
  store i32 2, ptr %init_ptr75, align 4
  %next_i76 = add i32 %i73, 1
  br label %init_loop_cond70

after_init72:                                     ; preds = %init_loop_cond70
  %arr277 = load [5 x i32], ptr %arr2, align 4
  %element_ptr78 = getelementptr [6 x [5 x i32]], ptr %mat2, i32 0, i32 3
  store [5 x i32] %arr277, ptr %element_ptr78, align 4
  %element_ptr79 = getelementptr [6 x [5 x i32]], ptr %mat2, i32 0, i32 3
  %element_ptr80 = getelementptr [5 x i32], ptr %element_ptr79, i32 0, i32 1
  %array_load81 = load i32, ptr %element_ptr80, align 4
  %15 = call i32 (ptr, ...) @printf(ptr @15, i32 %array_load81)
  %element_ptr82 = getelementptr [6 x [5 x i32]], ptr %mat2, i32 0, i32 2
  %element_ptr83 = getelementptr [5 x i32], ptr %element_ptr82, i32 0, i32 1
  %array_load84 = load i32, ptr %element_ptr83, align 4
  %16 = call i32 (ptr, ...) @printf(ptr @16, i32 %array_load84)
  ret i32 0
}

declare i32 @printf(ptr, ...)

declare i32 @scanf(ptr, ...)
