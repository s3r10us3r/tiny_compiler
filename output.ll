; ModuleID = 'TinyCompiler'
source_filename = "TinyCompiler"

%Vec2 = type { i32, i32 }

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @Vec2_dot(ptr %self, %Vec2 %other) {
entry:
  %other1 = alloca %Vec2, align 8
  store %Vec2 %other, ptr %other1, align 4
  %x_ptr = getelementptr inbounds nuw %Vec2, ptr %self, i32 0, i32 0
  %x_load = load i32, ptr %x_ptr, align 4
  %x_ptr2 = getelementptr inbounds nuw %Vec2, ptr %other1, i32 0, i32 0
  %x_load3 = load i32, ptr %x_ptr2, align 4
  %multmp = mul i32 %x_load, %x_load3
  %y_ptr = getelementptr inbounds nuw %Vec2, ptr %self, i32 0, i32 1
  %y_load = load i32, ptr %y_ptr, align 4
  %y_ptr4 = getelementptr inbounds nuw %Vec2, ptr %other1, i32 0, i32 1
  %y_load5 = load i32, ptr %y_ptr4, align 4
  %multmp6 = mul i32 %y_load, %y_load5
  %addtmp = add i32 %multmp, %multmp6
  ret i32 %addtmp

after_return:                                     ; No predecessors!
  ret i32 0
}

define i32 @main() {
entry:
  %a = alloca %Vec2, align 8
  %struct_tmp = alloca %Vec2, align 8
  %x_ptr = getelementptr inbounds nuw %Vec2, ptr %struct_tmp, i32 0, i32 0
  store i32 3, ptr %x_ptr, align 4
  %y_ptr = getelementptr inbounds nuw %Vec2, ptr %struct_tmp, i32 0, i32 1
  store i32 4, ptr %y_ptr, align 4
  %struct_val = load %Vec2, ptr %struct_tmp, align 4
  store %Vec2 %struct_val, ptr %a, align 4
  %b = alloca %Vec2, align 8
  %struct_tmp1 = alloca %Vec2, align 8
  %x_ptr2 = getelementptr inbounds nuw %Vec2, ptr %struct_tmp1, i32 0, i32 0
  store i32 1, ptr %x_ptr2, align 4
  %y_ptr3 = getelementptr inbounds nuw %Vec2, ptr %struct_tmp1, i32 0, i32 1
  store i32 2, ptr %y_ptr3, align 4
  %struct_val4 = load %Vec2, ptr %struct_tmp1, align 4
  store %Vec2 %struct_val4, ptr %b, align 4
  %b5 = load %Vec2, ptr %b, align 4
  %method_tmp = call i32 @Vec2_dot(ptr %a, %Vec2 %b5)
  %0 = call i32 (ptr, ...) @printf(ptr @0, i32 %method_tmp)
  ret i32 0
}

declare i32 @printf(ptr, ...)
