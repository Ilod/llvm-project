; RUN: opt < %s -asan -S \
; RUN:     -mtriple=i386-unknown-freebsd \
; RUN:     -default-data-layout="e-m:e-p:32:32-f64:32:64-f80:32-n8:16:32-S128" | \
; RUN:     FileCheck --check-prefix=CHECK-32 %s

; RUN: opt < %s -asan -S \
; RUN:     -mtriple=x86_64-unknown-freebsd \
; RUN:     -default-data-layout="e-m:e-i64:64-f80:128-n8:16:32:64-S128" | \
; RUN:     FileCheck --check-prefix=CHECK-64 %s

define i32 @read_4_bytes(i32* %a) sanitize_address {
entry:
  %tmp1 = load i32* %a, align 4
  ret i32 %tmp1
}

; CHECK-32: @read_4_bytes
; CHECK-32-NOT: ret
; Check for ASAN's Offset for 32-bit (2^30 or 0x40000000)
; CHECK-32: lshr {{.*}} 3
; CHECK-32-NEXT: {{1073741824}}
; CHECK-32: ret

; CHECK-64: @read_4_bytes
; CHECK-64-NOT: ret
; Check for ASAN's Offset for 64-bit (2^46 or 0x400000000000)
; CHECK-64: lshr {{.*}} 3
; CHECK-64-NEXT: {{70368744177664}}
; CHECK-64: ret
