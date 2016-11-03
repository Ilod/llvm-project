//===--- CodeGenPluginEntryPoint.cpp - Emit LLVM Code from ASTs ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This builds an AST and converts it to LLVM Code.
//
//===----------------------------------------------------------------------===//

#include "clang/CodeGen/CodeGenPluginEntryPoint.h"
#include "CodeGenModule.h"

using namespace clang;
using namespace CodeGen;

namespace clang {

llvm::Value *CodeGenPluginEntryPoint::GetAddrOfConstantStringFromLiteral(
                                const StringLiteral *S, StringRef Name) const {
  return CGM.GetAddrOfConstantStringFromLiteral(S, Name).getPointer();
}

llvm::Constant *CodeGenPluginEntryPoint::GetAddrOfGlobal(GlobalDecl GD,
                                                  bool IsForDefinition) const {
  return CGM.GetAddrOfGlobal(GD, IsForDefinition ? CodeGen::ForDefinition : CodeGen::NotForDefinition);
}

}
