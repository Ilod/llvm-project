; RUN: llc < %s -march=x86-64 | FileCheck %s -check-prefix=X64
; RUN: llc < %s -march=x86    | FileCheck %s -check-prefix=X32

%struct.Baz = type { [17 x i8] }
%struct.__va_list_tag = type { i32, i32, i8*, i8* }

; Function Attrs: nounwind uwtable
define void @bar(%struct.Baz* byval nocapture readnone align 8 %x, ...) {
entry:
  %va = alloca [1 x %struct.__va_list_tag], align 16
  %arraydecay = getelementptr inbounds [1 x %struct.__va_list_tag]* %va, i64 0, i64 0
  %arraydecay1 = bitcast [1 x %struct.__va_list_tag]* %va to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  %overflow_arg_area_p = getelementptr inbounds [1 x %struct.__va_list_tag]* %va, i64 0, i64 0, i32 2
  %overflow_arg_area = load i8** %overflow_arg_area_p, align 8
  %overflow_arg_area.next = getelementptr i8* %overflow_arg_area, i64 24
  store i8* %overflow_arg_area.next, i8** %overflow_arg_area_p, align 8
; X32: leal    68(%esp), [[REG:%.*]]
; X32: movl    [[REG]], 16(%esp)
; X64: leaq    232(%rsp), [[REG:%.*]]
; X64: movq    [[REG]], 184(%rsp)
; X64: leaq    176(%rsp), %rdi
  call void @qux(%struct.__va_list_tag* %arraydecay)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.va_start(i8*)

declare void @qux(%struct.__va_list_tag*)
