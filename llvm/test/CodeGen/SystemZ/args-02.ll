; Test the handling of GPR, FPR and stack arguments when integers are
; sign-extended.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-INT
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-FLOAT
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-DOUBLE
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-FP128-1
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-FP128-2
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s -check-prefix=CHECK-STACK

declare void @bar(i8 signext, i16 signext, i32 signext, i64, float, double,
                  fp128, i64, float, double, i8 signext, i16 signext,
                  i32 signext, i64, float, double, fp128)

; There are two indirect fp128 slots, one at offset 224 (the first available
; byte after the outgoing arguments) and one immediately after it at 240.
; These slots should be set up outside the glued call sequence, so would
; normally use %f0/%f2 as the first available 128-bit pair.  This choice
; is hard-coded in the FP128 tests.
;
; The order of the CHECK-INT loads doesn't matter.  The same goes for the
; CHECK_FP128-* stores and the CHECK-STACK stores.  It would be OK to reorder
; them in response to future code changes.
define void @foo() {
; CHECK-INT: foo:
; CHECK-INT: lghi %r2, -1
; CHECK-INT: lghi %r3, -2
; CHECK-INT: lghi %r4, -3
; CHECK-INT: lghi %r5, -4
; CHECK-INT: la %r6, {{224|240}}(%r15)
; CHECK-INT: brasl %r14, bar@PLT
;
; CHECK-FLOAT: foo:
; CHECK-FLOAT: lzer %f0
; CHECK-FLOAT: lcebr %f4, %f0
; CHECK-FLOAT: brasl %r14, bar@PLT
;
; CHECK-DOUBLE: foo:
; CHECK-DOUBLE: lzdr %f2
; CHECK-DOUBLE: lcdbr %f6, %f2
; CHECK-DOUBLE: brasl %r14, bar@PLT
;
; CHECK-FP128-1: foo:
; CHECK-FP128-1: aghi %r15, -256
; CHECK-FP128-1: lzxr %f0
; CHECK-FP128-1: std %f0, 224(%r15)
; CHECK-FP128-1: std %f2, 232(%r15)
; CHECK-FP128-1: brasl %r14, bar@PLT
;
; CHECK-FP128-2: foo:
; CHECK-FP128-2: aghi %r15, -256
; CHECK-FP128-2: lzxr %f0
; CHECK-FP128-2: std %f0, 240(%r15)
; CHECK-FP128-2: std %f2, 248(%r15)
; CHECK-FP128-2: brasl %r14, bar@PLT
;
; CHECK-STACK: foo:
; CHECK-STACK: aghi %r15, -256
; CHECK-STACK: la [[REGISTER:%r[0-5]+]], {{224|240}}(%r15)
; CHECK-STACK: stg [[REGISTER]], 216(%r15)
; CHECK-STACK: mvghi 208(%r15), 0
; CHECK-STACK: mvhi 204(%r15), 0
; CHECK-STACK: mvghi 192(%r15), -9
; CHECK-STACK: mvghi 184(%r15), -8
; CHECK-STACK: mvghi 176(%r15), -7
; CHECK-STACK: mvghi 168(%r15), -6
; CHECK-STACK: mvghi 160(%r15), -5
; CHECK-STACK: brasl %r14, bar@PLT

  call void @bar (i8 -1, i16 -2, i32 -3, i64 -4, float 0.0, double 0.0,
                  fp128 0xL00000000000000000000000000000000, i64 -5,
                  float -0.0, double -0.0, i8 -6, i16 -7, i32 -8, i64 -9,
                  float 0.0, double 0.0,
                  fp128 0xL00000000000000000000000000000000)
  ret void
}
