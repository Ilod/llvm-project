; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown | FileCheck %s --check-prefix=X32
; RUN: llc < %s -mtriple=x86_64-unknown | FileCheck %s --check-prefix=X64

; Use movzbl to avoid partial-register updates.

define i32 @foo(i32 %p, i8 zeroext %x) nounwind {
; X32-LABEL: foo:
; X32:       # BB#0:
; X32-NEXT:    movzbl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    divb {{[0-9]+}}(%esp)
; X32-NEXT:    movzbl %al, %eax
; X32-NEXT:    andl $1, %eax
; X32-NEXT:    retl
;
; X64-LABEL: foo:
; X64:       # BB#0:
; X64-NEXT:    movzbl %dil, %eax
; X64-NEXT:    divb %sil
; X64-NEXT:    movzbl %al, %eax
; X64-NEXT:    andl $1, %eax
; X64-NEXT:    retq
  %q = trunc i32 %p to i8
  %r = udiv i8 %q, %x
  %s = zext i8 %r to i32
  %t = and i32 %s, 1
  ret i32 %t
}

define i32 @bar(i32 %p, i16 zeroext %x) nounwind {
; X32-LABEL: bar:
; X32:       # BB#0:
; X32-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    xorl %edx, %edx
; X32-NEXT:    divw {{[0-9]+}}(%esp)
; X32-NEXT:    andl $1, %eax
; X32-NEXT:    retl
;
; X64-LABEL: bar:
; X64:       # BB#0:
; X64-NEXT:    xorl %edx, %edx
; X64-NEXT:    movw %di, %ax
; X64-NEXT:    divw %si
; X64-NEXT:    andl $1, %eax
; X64-NEXT:    retq
  %q = trunc i32 %p to i16
  %r = udiv i16 %q, %x
  %s = zext i16 %r to i32
  %t = and i32 %s, 1
  ret i32 %t
}
