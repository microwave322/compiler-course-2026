; RUN: opt -load-pass-plugin %llvmshlibdir/LoadStoreEliminationPass_Guseva_Alena_FIIT2_LLVM_IR%pluginext \
; RUN: -passes=load-store-elim -S %s | FileCheck %s

declare void @foo(ptr)

; CHECK-LABEL: @_test1
; CHECK-NEXT: store i32 %1, ptr %0, align 4
; CHECK-NEXT: ret i32 %1
; CHECK-NOT: load i32

define dso_local i32 @_test1(ptr %0, i32 %1) {
  store i32 %1, ptr %0, align 4
  %x = load i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test2
; CHECK-NEXT: store i32 42, ptr %0, align 4
; CHECK-NEXT: ret i32 42
; CHECK-NOT: load i32

define dso_local i32 @_test2(ptr %0) {
  store i32 42, ptr %0, align 4
  %x = load i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test3
; CHECK-NEXT: store i32 2, ptr %0, align 4
; CHECK-NEXT: ret i32 2
; CHECK-NOT: store i32 1
; CHECK-NOT: load i32

define dso_local i32 @_test3(ptr %0) {
  store i32 1, ptr %0, align 4
  store i32 2, ptr %0, align 4
  %x = load i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test4
; CHECK-NEXT: store i32 1, ptr %0, align 4
; CHECK-NEXT: store i32 2, ptr %0, align 4
; CHECK-NEXT: %sum = add i32 1, 2
; CHECK-NEXT: ret i32 %sum
; CHECK-NOT: %a = load i32

define dso_local i32 @_test4(ptr %0) {
  store i32 1, ptr %0, align 4
  %a = load i32, ptr %0, align 4
  store i32 2, ptr %0, align 4
  %sum = add i32 %a, 2
  ret i32 %sum
}

; CHECK-LABEL: @_test5
; CHECK-NEXT: store i32 %1, ptr %0, align 4
; CHECK-NEXT: %sum = add i32 %1, %1
; CHECK-NEXT: ret i32 %sum
; CHECK-NOT: load i32

define dso_local i32 @_test5(ptr %0, i32 %1) {
  store i32 %1, ptr %0, align 4
  %a = load i32, ptr %0, align 4
  %b = load i32, ptr %0, align 4
  %sum = add i32 %a, %b
  ret i32 %sum
}

; CHECK-LABEL: @_test6
; CHECK-NEXT: store volatile i32 5, ptr %0, align 4
; CHECK-NEXT: %x = load volatile i32, ptr %0, align 4
; CHECK-NEXT: ret i32 %x
; CHECK-NOT: ret i32 5

define dso_local i32 @_test6(ptr %0) {
  store volatile i32 5, ptr %0, align 4
  %x = load volatile i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test7
; CHECK-NEXT: store i32 9, ptr %0, align 4
; CHECK-NEXT: call void @foo(ptr %0)
; CHECK-NEXT: %x = load i32, ptr %0, align 4
; CHECK-NEXT: ret i32 %x
; CHECK-NOT: ret i32 9

define dso_local i32 @_test7(ptr %0) {
  store i32 9, ptr %0, align 4
  call void @foo(ptr %0)
  %x = load i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test8
; CHECK-NEXT: store i32 1, ptr %0, align 4
; CHECK-NEXT: store i32 2, ptr %1, align 4
; CHECK-NEXT: ret i32 1
; CHECK-NOT: load i32

define dso_local i32 @_test8(ptr %0, ptr %1) {
  store i32 1, ptr %0, align 4
  store i32 2, ptr %1, align 4
  %x = load i32, ptr %0, align 4
  ret i32 %x
}

; CHECK-LABEL: @_test9_float
; CHECK-NEXT: store float %1, ptr %0, align 4
; CHECK-NEXT: ret float %1
; CHECK-NOT: load float

define dso_local float @_test9_float(ptr %0, float %1) {
  store float %1, ptr %0, align 4
  %x = load float, ptr %0, align 4
  ret float %x
}
