//===--- CodeGen/ModuleBuilder.h - Build LLVM from ASTs ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the ModuleBuilder interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_CODEGEN_PLUGINENTRYPOINT_H
#define LLVM_CLANG_CODEGEN_PLUGINENTRYPOINT_H

#include "llvm/ADT/StringRef.h"

namespace llvm {
  class Constant;
  class Value;
}

namespace clang {
  class GlobalDecl;
  class PreprocessorOptions;
  class StringLiteral;

namespace CodeGen {
  class CodeGenModule;
}

/// The primary public interface to the Clang code generator.
///
/// This is not really an abstract interface.
class CodeGenPluginEntryPoint {
public:
  CodeGenPluginEntryPoint(CodeGen::CodeGenModule& cgm) : CGM(cgm) {}
  CodeGen::CodeGenModule& CGM;
  /// Return a pointer to a constant array for the given string literal.
  llvm::Value* GetAddrOfConstantStringFromLiteral(const StringLiteral *S,
                                     llvm::StringRef Name = ".str") const;
  llvm::Constant *GetAddrOfGlobal(GlobalDecl GD,
                                  bool IsForDefinition = false) const;
};

} // end namespace clang

#endif
