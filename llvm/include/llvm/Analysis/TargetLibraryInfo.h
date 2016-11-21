//===-- TargetLibraryInfo.h - Library information ---------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_TARGETLIBRARYINFO_H
#define LLVM_ANALYSIS_TARGETLIBRARYINFO_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {
template <typename T> class ArrayRef;

/// Describes a possible vectorization of a function.
/// Function 'VectorFnName' is equivalent to 'ScalarFnName' vectorized
/// by a factor 'VectorizationFactor'.
struct VecDesc {
  StringRef ScalarFnName;
  StringRef VectorFnName;
  unsigned VectorizationFactor;
};

  namespace LibFunc {
    enum Func {
#define TLI_DEFINE_ENUM
#include "llvm/Analysis/TargetLibraryInfo.def"

      NumLibFuncs
    };
  }

/// Implementation of the target library information.
///
/// This class constructs tables that hold the target library information and
/// make it available. However, it is somewhat expensive to compute and only
/// depends on the triple. So users typically interact with the \c
/// TargetLibraryInfo wrapper below.
class TargetLibraryInfoImpl {
  friend class TargetLibraryInfo;

  unsigned char AvailableArray[(LibFunc::NumLibFuncs+3)/4];
  llvm::DenseMap<unsigned, std::string> CustomNames;
  static StringRef const StandardNames[LibFunc::NumLibFuncs];
  bool ShouldExtI32Param, ShouldExtI32Return, ShouldSignExtI32Param;

  enum AvailabilityState {
    StandardName = 3, // (memset to all ones)
    CustomName = 1,
    Unavailable = 0  // (memset to all zeros)
  };
  void setState(LibFunc::Func F, AvailabilityState State) {
    AvailableArray[F/4] &= ~(3 << 2*(F&3));
    AvailableArray[F/4] |= State << 2*(F&3);
  }
  AvailabilityState getState(LibFunc::Func F) const {
    return static_cast<AvailabilityState>((AvailableArray[F/4] >> 2*(F&3)) & 3);
  }

  /// Vectorization descriptors - sorted by ScalarFnName.
  std::vector<VecDesc> VectorDescs;
  /// Scalarization descriptors - same content as VectorDescs but sorted based
  /// on VectorFnName rather than ScalarFnName.
  std::vector<VecDesc> ScalarDescs;

  /// Return true if the function type FTy is valid for the library function
  /// F, regardless of whether the function is available.
  bool isValidProtoForLibFunc(const FunctionType &FTy, LibFunc::Func F,
                              const DataLayout *DL) const;

public:
  /// List of known vector-functions libraries.
  ///
  /// The vector-functions library defines, which functions are vectorizable
  /// and with which factor. The library can be specified by either frontend,
  /// or a commandline option, and then used by
  /// addVectorizableFunctionsFromVecLib for filling up the tables of
  /// vectorizable functions.
  enum VectorLibrary {
    NoLibrary,  // Don't use any vector library.
    Accelerate, // Use Accelerate framework.
    SVML        // Intel short vector math library.
  };

  TargetLibraryInfoImpl();
  explicit TargetLibraryInfoImpl(const Triple &T);

  // Provide value semantics.
  TargetLibraryInfoImpl(const TargetLibraryInfoImpl &TLI);
  TargetLibraryInfoImpl(TargetLibraryInfoImpl &&TLI);
  TargetLibraryInfoImpl &operator=(const TargetLibraryInfoImpl &TLI);
  TargetLibraryInfoImpl &operator=(TargetLibraryInfoImpl &&TLI);

  /// Searches for a particular function name.
  ///
  /// If it is one of the known library functions, return true and set F to the
  /// corresponding value.
  bool getLibFunc(StringRef funcName, LibFunc::Func &F) const;

  /// Searches for a particular function name, also checking that its type is
  /// valid for the library function matching that name.
  ///
  /// If it is one of the known library functions, return true and set F to the
  /// corresponding value.
  bool getLibFunc(const Function &FDecl, LibFunc::Func &F) const;

  /// Forces a function to be marked as unavailable.
  void setUnavailable(LibFunc::Func F) {
    setState(F, Unavailable);
  }

  /// Forces a function to be marked as available.
  void setAvailable(LibFunc::Func F) {
    setState(F, StandardName);
  }

  /// Forces a function to be marked as available and provide an alternate name
  /// that must be used.
  void setAvailableWithName(LibFunc::Func F, StringRef Name) {
    if (StandardNames[F] != Name) {
      setState(F, CustomName);
      CustomNames[F] = Name;
      assert(CustomNames.find(F) != CustomNames.end());
    } else {
      setState(F, StandardName);
    }
  }

  /// Disables all builtins.
  ///
  /// This can be used for options like -fno-builtin.
  void disableAllFunctions();

  /// Add a set of scalar -> vector mappings, queryable via
  /// getVectorizedFunction and getScalarizedFunction.
  void addVectorizableFunctions(ArrayRef<VecDesc> Fns);

  /// Calls addVectorizableFunctions with a known preset of functions for the
  /// given vector library.
  void addVectorizableFunctionsFromVecLib(enum VectorLibrary VecLib);

  /// Return true if the function F has a vector equivalent with vectorization
  /// factor VF.
  bool isFunctionVectorizable(StringRef F, unsigned VF) const {
    return !getVectorizedFunction(F, VF).empty();
  }

  /// Return true if the function F has a vector equivalent with any
  /// vectorization factor.
  bool isFunctionVectorizable(StringRef F) const;

  /// Return the name of the equivalent of F, vectorized with factor VF. If no
  /// such mapping exists, return the empty string.
  StringRef getVectorizedFunction(StringRef F, unsigned VF) const;

  /// Return true if the function F has a scalar equivalent, and set VF to be
  /// the vectorization factor.
  bool isFunctionScalarizable(StringRef F, unsigned &VF) const {
    return !getScalarizedFunction(F, VF).empty();
  }

  /// Return the name of the equivalent of F, scalarized. If no such mapping
  /// exists, return the empty string.
  ///
  /// Set VF to the vectorization factor.
  StringRef getScalarizedFunction(StringRef F, unsigned &VF) const;

  /// Set to true iff i32 parameters to library functions should have signext
  /// or zeroext attributes if they correspond to C-level int or unsigned int,
  /// respectively.
  void setShouldExtI32Param(bool Val) {
    ShouldExtI32Param = Val;
  }

  /// Set to true iff i32 results from library functions should have signext
  /// or zeroext attributes if they correspond to C-level int or unsigned int,
  /// respectively.
  void setShouldExtI32Return(bool Val) {
    ShouldExtI32Return = Val;
  }

  /// Set to true iff i32 parameters to library functions should have signext
  /// attribute if they correspond to C-level int or unsigned int.
  void setShouldSignExtI32Param(bool Val) {
    ShouldSignExtI32Param = Val;
  }
};

/// Provides information about what library functions are available for
/// the current target.
///
/// This both allows optimizations to handle them specially and frontends to
/// disable such optimizations through -fno-builtin etc.
class TargetLibraryInfo {
  friend class TargetLibraryAnalysis;
  friend class TargetLibraryInfoWrapperPass;

  const TargetLibraryInfoImpl *Impl;

public:
  explicit TargetLibraryInfo(const TargetLibraryInfoImpl &Impl) : Impl(&Impl) {}

  // Provide value semantics.
  TargetLibraryInfo(const TargetLibraryInfo &TLI) : Impl(TLI.Impl) {}
  TargetLibraryInfo(TargetLibraryInfo &&TLI) : Impl(TLI.Impl) {}
  TargetLibraryInfo &operator=(const TargetLibraryInfo &TLI) {
    Impl = TLI.Impl;
    return *this;
  }
  TargetLibraryInfo &operator=(TargetLibraryInfo &&TLI) {
    Impl = TLI.Impl;
    return *this;
  }

  /// Searches for a particular function name.
  ///
  /// If it is one of the known library functions, return true and set F to the
  /// corresponding value.
  bool getLibFunc(StringRef funcName, LibFunc::Func &F) const {
    return Impl->getLibFunc(funcName, F);
  }

  bool getLibFunc(const Function &FDecl, LibFunc::Func &F) const {
    return Impl->getLibFunc(FDecl, F);
  }

  /// Tests whether a library function is available.
  bool has(LibFunc::Func F) const {
    return Impl->getState(F) != TargetLibraryInfoImpl::Unavailable;
  }
  bool isFunctionVectorizable(StringRef F, unsigned VF) const {
    return Impl->isFunctionVectorizable(F, VF);
  }
  bool isFunctionVectorizable(StringRef F) const {
    return Impl->isFunctionVectorizable(F);
  }
  StringRef getVectorizedFunction(StringRef F, unsigned VF) const {
    return Impl->getVectorizedFunction(F, VF);
  }

  /// Tests if the function is both available and a candidate for optimized code
  /// generation.
  bool hasOptimizedCodeGen(LibFunc::Func F) const {
    if (Impl->getState(F) == TargetLibraryInfoImpl::Unavailable)
      return false;
    switch (F) {
    default: break;
    case LibFunc::copysign:  case LibFunc::copysignf:  case LibFunc::copysignl:
    case LibFunc::fabs:      case LibFunc::fabsf:      case LibFunc::fabsl:
    case LibFunc::sin:       case LibFunc::sinf:       case LibFunc::sinl:
    case LibFunc::cos:       case LibFunc::cosf:       case LibFunc::cosl:
    case LibFunc::sqrt:      case LibFunc::sqrtf:      case LibFunc::sqrtl:
    case LibFunc::sqrt_finite: case LibFunc::sqrtf_finite:
                                                  case LibFunc::sqrtl_finite:
    case LibFunc::fmax:      case LibFunc::fmaxf:      case LibFunc::fmaxl:
    case LibFunc::fmin:      case LibFunc::fminf:      case LibFunc::fminl:
    case LibFunc::floor:     case LibFunc::floorf:     case LibFunc::floorl:
    case LibFunc::nearbyint: case LibFunc::nearbyintf: case LibFunc::nearbyintl:
    case LibFunc::ceil:      case LibFunc::ceilf:      case LibFunc::ceill:
    case LibFunc::rint:      case LibFunc::rintf:      case LibFunc::rintl:
    case LibFunc::round:     case LibFunc::roundf:     case LibFunc::roundl:
    case LibFunc::trunc:     case LibFunc::truncf:     case LibFunc::truncl:
    case LibFunc::log2:      case LibFunc::log2f:      case LibFunc::log2l:
    case LibFunc::exp2:      case LibFunc::exp2f:      case LibFunc::exp2l:
    case LibFunc::memcmp:    case LibFunc::strcmp:     case LibFunc::strcpy:
    case LibFunc::stpcpy:    case LibFunc::strlen:     case LibFunc::strnlen:
    case LibFunc::memchr:    case LibFunc::mempcpy:
      return true;
    }
    return false;
  }

  StringRef getName(LibFunc::Func F) const {
    auto State = Impl->getState(F);
    if (State == TargetLibraryInfoImpl::Unavailable)
      return StringRef();
    if (State == TargetLibraryInfoImpl::StandardName)
      return Impl->StandardNames[F];
    assert(State == TargetLibraryInfoImpl::CustomName);
    return Impl->CustomNames.find(F)->second;
  }

  /// Returns extension attribute kind to be used for i32 parameters
  /// correpsonding to C-level int or unsigned int.  May be zeroext, signext,
  /// or none.
  Attribute::AttrKind getExtAttrForI32Param(bool Signed = true) const {
    if (Impl->ShouldExtI32Param)
      return Signed ? Attribute::SExt : Attribute::ZExt;
    if (Impl->ShouldSignExtI32Param)
      return Attribute::SExt;
    return Attribute::None;
  }

  /// Returns extension attribute kind to be used for i32 return values
  /// correpsonding to C-level int or unsigned int.  May be zeroext, signext,
  /// or none.
  Attribute::AttrKind getExtAttrForI32Return(bool Signed = true) const {
    if (Impl->ShouldExtI32Return)
      return Signed ? Attribute::SExt : Attribute::ZExt;
    return Attribute::None;
  }

  /// Handle invalidation from the pass manager.
  ///
  /// If we try to invalidate this info, just return false. It cannot become
  /// invalid even if the module or function changes.
  bool invalidate(Module &, const PreservedAnalyses &) { return false; }
  bool invalidate(Function &, const PreservedAnalyses &) { return false; }
};

/// Analysis pass providing the \c TargetLibraryInfo.
///
/// Note that this pass's result cannot be invalidated, it is immutable for the
/// life of the module.
class TargetLibraryAnalysis : public AnalysisInfoMixin<TargetLibraryAnalysis> {
public:
  typedef TargetLibraryInfo Result;

  /// Default construct the library analysis.
  ///
  /// This will use the module's triple to construct the library info for that
  /// module.
  TargetLibraryAnalysis() {}

  /// Construct a library analysis with preset info.
  ///
  /// This will directly copy the preset info into the result without
  /// consulting the module's triple.
  TargetLibraryAnalysis(TargetLibraryInfoImpl PresetInfoImpl)
      : PresetInfoImpl(std::move(PresetInfoImpl)) {}

  TargetLibraryInfo run(Module &M, ModuleAnalysisManager &);
  TargetLibraryInfo run(Function &F, FunctionAnalysisManager &);

private:
  friend AnalysisInfoMixin<TargetLibraryAnalysis>;
  static char PassID;

  Optional<TargetLibraryInfoImpl> PresetInfoImpl;

  StringMap<std::unique_ptr<TargetLibraryInfoImpl>> Impls;

  TargetLibraryInfoImpl &lookupInfoImpl(const Triple &T);
};

class TargetLibraryInfoWrapperPass : public ImmutablePass {
  TargetLibraryInfoImpl TLIImpl;
  TargetLibraryInfo TLI;

  virtual void anchor();

public:
  static char ID;
  TargetLibraryInfoWrapperPass();
  explicit TargetLibraryInfoWrapperPass(const Triple &T);
  explicit TargetLibraryInfoWrapperPass(const TargetLibraryInfoImpl &TLI);

  TargetLibraryInfo &getTLI() { return TLI; }
  const TargetLibraryInfo &getTLI() const { return TLI; }
};

} // end namespace llvm

#endif
