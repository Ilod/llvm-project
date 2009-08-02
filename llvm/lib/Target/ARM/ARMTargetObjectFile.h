//===-- llvm/Target/ARMTargetObjectFile.h - ARM Object Info -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_ARM_TARGETOBJECTFILE_H
#define LLVM_TARGET_ARM_TARGETOBJECTFILE_H

#include "llvm/Target/TargetLoweringObjectFile.h"

namespace llvm {
  
  class ARMElfTargetObjectFile : public TargetLoweringObjectFileELF {
  public:
    ARMElfTargetObjectFile() : TargetLoweringObjectFileELF(true) {}
    
    void Initialize(MCContext &Ctx, const TargetMachine &TM) {
      TargetLoweringObjectFileELF::Initialize(Ctx, TM);
      
      // FIXME: Add new attribute/flag to MCSection for init_array/fini_array.
      // That will allow not treating these as "directives".
      if (TM.getSubtarget<ARMSubtarget>().isAAPCS_ABI()) {
        StaticCtorSection =
          getOrCreateSection("\t.section .init_array,\"aw\",%init_array", false,
                             SectionKind::getDataRel());
        StaticDtorSection =
          getOrCreateSection("\t.section .fini_array,\"aw\",%fini_array", false,
                             SectionKind::getDataRel());
      }
    }
  };
} // end namespace llvm

#endif
