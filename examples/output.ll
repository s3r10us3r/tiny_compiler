; ModuleID = 'TinyCompiler'
source_filename = "TinyCompiler"

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@2 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@4 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@5 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@6 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@7 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@8 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
entry:
  %a = alloca i32, align 4
  store i32 -4, ptr %a, align 4
  %b = alloca i32, align 4
  store i32 10, ptr %b, align 4
  %a1 = load i32, ptr %a, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @0, i32 %a1)
  %c = alloca i32, align 4
  %a2 = load i32, ptr %a, align 4
  %b3 = load i32, ptr %b, align 4
  %multmp = mul i32 %b3, 2
  %addtmp = add i32 %a2, %multmp
  store i32 %addtmp, ptr %c, align 4
  %c4 = load i32, ptr %c, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @1, i32 %c4)
  %c5 = load i32, ptr %c, align 4
  %subtmp = sub i32 %c5, 5
  store i32 %subtmp, ptr %c, align 4
  %c6 = load i32, ptr %c, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @2, i32 %c6)
  %pi = alloca double, align 8
  store double 3.140000e+00, ptr %pi, align 8
  %r = alloca double, align 8
  store double 2.000000e+00, ptr %r, align 8
  %area = alloca double, align 8
  %pi7 = load double, ptr %pi, align 8
  %r8 = load double, ptr %r, align 8
  %multmp9 = fmul double %pi7, %r8
  %r10 = load double, ptr %r, align 8
  %multmp11 = fmul double %multmp9, %r10
  store double %multmp11, ptr %area, align 8
  %area12 = load double, ptr %area, align 8
  %3 = call i32 (ptr, ...) @printf(ptr @3, double %area12)
  %no_brackets = alloca i32, align 4
  store i32 -10, ptr %no_brackets, align 4
  %brackets = alloca i32, align 4
  store i32 10, ptr %brackets, align 4
  %no_brackets13 = load i32, ptr %no_brackets, align 4
  %4 = call i32 (ptr, ...) @printf(ptr @4, i32 %no_brackets13)
  %brackets14 = load i32, ptr %brackets, align 4
  %5 = call i32 (ptr, ...) @printf(ptr @5, i32 %brackets14)
  %dived = alloca double, align 8
  store double 0x400AAAAAAAAAAAAB, ptr %dived, align 8
  %dived15 = load double, ptr %dived, align 8
  %6 = call i32 (ptr, ...) @printf(ptr @6, double %dived15)
  %7 = call i32 (ptr, ...) @scanf(ptr @7, ptr %a)
  %a16 = load i32, ptr %a, align 4
  %multmp17 = mul i32 %a16, 10
  %8 = call i32 (ptr, ...) @printf(ptr @8, i32 %multmp17)
  ret i32 0
}

declare i32 @printf(ptr, ...)

declare i32 @scanf(ptr, ...)
