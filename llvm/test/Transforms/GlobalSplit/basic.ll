; RUN: opt -S -globalsplit %s | FileCheck %s
; RUN: opt -S -passes=globalsplit %s | FileCheck %s

target datalayout = "e-p:64:64"
target triple = "x86_64-unknown-linux-gnu"

; CHECK: @vtt = constant [3 x i8*] [i8* bitcast ([2 x i8* ()*]* @global.0 to i8*), i8* bitcast (i8* ()** getelementptr inbounds ([2 x i8* ()*], [2 x i8* ()*]* @global.0, i32 0, i32 1) to i8*), i8* bitcast ([1 x i8* ()*]* @global.1 to i8*)]
@vtt = constant [3 x i8*] [
  i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 0, i32 0) to i8*),
  i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 0, i32 1) to i8*),
  i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 1, i32 0) to i8*)
]

; CHECK-NOT: @global =
; CHECK: @global.0 = private constant [2 x i8* ()*] [i8* ()* @f1, i8* ()* @f2], !type [[T1:![0-9]+$]]
; CHECK: @global.1 = private constant [1 x i8* ()*] [i8* ()* @f3], !type [[T2:![0-9]+$]]
; CHECK-NOT: @global =
@global = internal constant { [2 x i8* ()*], [1 x i8* ()*] } {
  [2 x i8* ()*] [i8* ()* @f1, i8* ()* @f2],
  [1 x i8* ()*] [i8* ()* @f3]
}, !type !0, !type !1

; CHECK: define i8* @f1()
define i8* @f1() {
  ; CHECK-NEXT: ret i8* bitcast ([2 x i8* ()*]* @global.0 to i8*)
  ret i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 0, i32 0) to i8*)
}

; CHECK: define i8* @f2()
define i8* @f2() {
  ; CHECK-NEXT: ret i8* bitcast (i8* ()** getelementptr inbounds ([2 x i8* ()*], [2 x i8* ()*]* @global.0, i32 0, i32 1) to i8*)
  ret i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 0, i32 1) to i8*)
}

; CHECK: define i8* @f3()
define i8* @f3() {
  ; CHECK-NEXT: ret i8* bitcast (i8* ()** getelementptr inbounds ([2 x i8* ()*], [2 x i8* ()*]* @global.0, i64 1, i32 0) to i8*)
  ret i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 0, i32 2) to i8*)
}

; CHECK: define i8* @f4()
define i8* @f4() {
  ; CHECK-NEXT: ret i8* bitcast ([1 x i8* ()*]* @global.1 to i8*)
  ret i8* bitcast (i8* ()** getelementptr ({ [2 x i8* ()*], [1 x i8* ()*] }, { [2 x i8* ()*], [1 x i8* ()*] }* @global, i32 0, inrange i32 1, i32 0) to i8*)
}

define void @foo() {
  %p = call i1 @llvm.type.test(i8* null, metadata !"")
  ret void
}

declare i1 @llvm.type.test(i8*, metadata) nounwind readnone

; CHECK: [[T1]] = !{i32 8, !"foo"}
; CHECK: [[T2]] = !{i32 0, !"bar"}
!0 = !{i32 8, !"foo"}
!1 = !{i32 16, !"bar"}
