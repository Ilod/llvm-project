; Make sure that for each supported architecture in RelocVisitor::visit,
; the visitor does compute the relocation.
; RUN: llc -filetype=obj -O0 < %s -mtriple x86_64-none-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple i386-none-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple powerpc64-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple powerpc-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple mips-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple mips64-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple arm-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple aarch64-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple s390x-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple sparc-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s
; RUN: llc -filetype=obj -O0 < %s -mtriple sparcv9-unknown-linux-eabi | llvm-dwarfdump - 2>&1 | FileCheck %s

; CHECK-NOT: failed to compute relocation

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!llvm.ident = !{!5}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.6.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !2, metadata !2, metadata !2, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/a/empty.c] [DW_LANG_C99]
!1 = metadata !{metadata !"empty.c", metadata !"/a"}
!2 = metadata !{}
!3 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!4 = metadata !{i32 2, metadata !"Debug Info Version", i32 1}
!5 = metadata !{metadata !"clang version 3.6.0 "}
