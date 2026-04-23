; ModuleID = 'TinyCompiler'
source_filename = "TinyCompiler"

@str_tmp = private unnamed_addr constant [13 x i8] c"Hello World!\00", align 1
@0 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@str_tmp.1 = private unnamed_addr constant [16 x i8] c"Czytamy string!\00", align 1
@1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@2 = private unnamed_addr constant [8 x i8] c" %m[^\0A]\00", align 1
@3 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

define i32 @main() {
entry:
  %a = alloca ptr, align 8
  store ptr @str_tmp, ptr %a, align 8
  %a1 = load ptr, ptr %a, align 8
  %0 = call i32 (ptr, ...) @printf(ptr @0, ptr %a1)
  %1 = call i32 (ptr, ...) @printf(ptr @1, ptr @str_tmp.1)
  %2 = call i32 (ptr, ...) @scanf(ptr @2, ptr %a)
  %a2 = load ptr, ptr %a, align 8
  %3 = call i32 (ptr, ...) @printf(ptr @3, ptr %a2)
  ret i32 0
}

declare i32 @printf(ptr, ...)

declare i32 @scanf(ptr, ...)
