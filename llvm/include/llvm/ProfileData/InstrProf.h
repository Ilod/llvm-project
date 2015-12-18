//=-- InstrProf.h - Instrumented profiling format support ---------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Instrumentation-based profiling data is generated by instrumented
// binaries through library functions in compiler-rt, and read by the clang
// frontend to feed PGO.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PROFILEDATA_INSTRPROF_H_
#define LLVM_PROFILEDATA_INSTRPROF_H_

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/ProfileData/InstrProfData.inc"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MD5.h"
#include <cstdint>
#include <list>
#include <system_error>
#include <vector>

#define INSTR_PROF_INDEX_VERSION 3
namespace llvm {

class Function;
class GlobalVariable;
class Module;

/// Return the name of data section containing profile counter variables.
inline StringRef getInstrProfCountersSectionName(bool AddSegment) {
  return AddSegment ? "__DATA," INSTR_PROF_CNTS_SECT_NAME_STR
                    : INSTR_PROF_CNTS_SECT_NAME_STR;
}

/// Return the name of data section containing names of instrumented
/// functions.
inline StringRef getInstrProfNameSectionName(bool AddSegment) {
  return AddSegment ? "__DATA," INSTR_PROF_NAME_SECT_NAME_STR
                    : INSTR_PROF_NAME_SECT_NAME_STR;
}

/// Return the name of the data section containing per-function control
/// data.
inline StringRef getInstrProfDataSectionName(bool AddSegment) {
  return AddSegment ? "__DATA," INSTR_PROF_DATA_SECT_NAME_STR
                    : INSTR_PROF_DATA_SECT_NAME_STR;
}

/// Return the name profile runtime entry point to do value profiling
/// for a given site.
inline StringRef getInstrProfValueProfFuncName() {
  return INSTR_PROF_VALUE_PROF_FUNC_STR;
}

/// Return the name of the section containing function coverage mapping
/// data.
inline StringRef getInstrProfCoverageSectionName(bool AddSegment) {
  return AddSegment ? "__DATA,__llvm_covmap" : "__llvm_covmap";
}

/// Return the name prefix of variables containing instrumented function names.
inline StringRef getInstrProfNameVarPrefix() { return "__profn_"; }

/// Return the name prefix of variables containing per-function control data.
inline StringRef getInstrProfDataVarPrefix() { return "__profd_"; }

/// Return the name prefix of profile counter variables.
inline StringRef getInstrProfCountersVarPrefix() { return "__profc_"; }

/// Return the name prefix of the COMDAT group for instrumentation variables
/// associated with a COMDAT function.
inline StringRef getInstrProfComdatPrefix() { return "__profv_"; }

/// Return the name of a covarage mapping variable (internal linkage)
/// for each instrumented source module. Such variables are allocated
/// in the __llvm_covmap section.
inline StringRef getCoverageMappingVarName() {
  return "__llvm_coverage_mapping";
}

/// Return the name of function that registers all the per-function control
/// data at program startup time by calling __llvm_register_function. This
/// function has internal linkage and is called by  __llvm_profile_init
/// runtime method. This function is not generated for these platforms:
/// Darwin, Linux, and FreeBSD.
inline StringRef getInstrProfRegFuncsName() {
  return "__llvm_profile_register_functions";
}

/// Return the name of the runtime interface that registers per-function control
/// data for one instrumented function.
inline StringRef getInstrProfRegFuncName() {
  return "__llvm_profile_register_function";
}

/// Return the name of the runtime initialization method that is generated by
/// the compiler. The function calls __llvm_profile_register_functions and
/// __llvm_profile_override_default_filename functions if needed. This function
/// has internal linkage and invoked at startup time via init_array.
inline StringRef getInstrProfInitFuncName() { return "__llvm_profile_init"; }

/// Return the name of the hook variable defined in profile runtime library.
/// A reference to the variable causes the linker to link in the runtime
/// initialization module (which defines the hook variable).
inline StringRef getInstrProfRuntimeHookVarName() {
  return "__llvm_profile_runtime";
}

/// Return the name of the compiler generated function that references the
/// runtime hook variable. The function is a weak global.
inline StringRef getInstrProfRuntimeHookVarUseFuncName() {
  return "__llvm_profile_runtime_user";
}

/// Return the name of the profile runtime interface that overrides the default
/// profile data file name.
inline StringRef getInstrProfFileOverriderFuncName() {
  return "__llvm_profile_override_default_filename";
}

/// Return the modified name for function \c F suitable to be
/// used the key for profile lookup.
std::string getPGOFuncName(const Function &F,
                           uint64_t Version = INSTR_PROF_INDEX_VERSION);

/// Return the modified name for a function suitable to be
/// used the key for profile lookup. The function's original
/// name is \c RawFuncName and has linkage of type \c Linkage.
/// The function is defined in module \c FileName.
std::string getPGOFuncName(StringRef RawFuncName,
                           GlobalValue::LinkageTypes Linkage,
                           StringRef FileName,
                           uint64_t Version = INSTR_PROF_INDEX_VERSION);

/// Create and return the global variable for function name used in PGO
/// instrumentation. \c FuncName is the name of the function returned
/// by \c getPGOFuncName call.
GlobalVariable *createPGOFuncNameVar(Function &F, StringRef FuncName);

/// Create and return the global variable for function name used in PGO
/// instrumentation.  /// \c FuncName is the name of the function
/// returned by \c getPGOFuncName call, \c M is the owning module,
/// and \c Linkage is the linkage of the instrumented function.
GlobalVariable *createPGOFuncNameVar(Module &M,
                                     GlobalValue::LinkageTypes Linkage,
                                     StringRef FuncName);

/// Given a PGO function name, remove the filename prefix and return
/// the original (static) function name.
StringRef getFuncNameWithoutPrefix(StringRef PGOFuncName, StringRef FileName);

const std::error_category &instrprof_category();

enum class instrprof_error {
  success = 0,
  eof,
  unrecognized_format,
  bad_magic,
  bad_header,
  unsupported_version,
  unsupported_hash_type,
  too_large,
  truncated,
  malformed,
  unknown_function,
  hash_mismatch,
  count_mismatch,
  counter_overflow,
  value_site_count_mismatch
};

inline std::error_code make_error_code(instrprof_error E) {
  return std::error_code(static_cast<int>(E), instrprof_category());
}

inline instrprof_error MergeResult(instrprof_error &Accumulator,
                                   instrprof_error Result) {
  // Prefer first error encountered as later errors may be secondary effects of
  // the initial problem.
  if (Accumulator == instrprof_error::success &&
      Result != instrprof_error::success)
    Accumulator = Result;
  return Accumulator;
}

enum InstrProfValueKind : uint32_t {
#define VALUE_PROF_KIND(Enumerator, Value) Enumerator = Value,
#include "llvm/ProfileData/InstrProfData.inc"
};

namespace object {
class SectionRef;
}
/// A symbol table used for function PGO name look-up with keys
/// (such as pointers, md5hash values) to the function. A function's
/// PGO name or name's md5hash are used in retrieving the profile
/// data of the function. See \c getPGOFuncName() method for details
/// on how PGO name is formed.
class InstrProfSymtab {
private:
  StringRef Data;
  uint64_t Address;

public:
  InstrProfSymtab() : Data(), Address(0) {}

  /// Create InstrProfSymtab from a object file section which
  /// contains function PGO names that are uncompressed.
  std::error_code create(object::SectionRef &Section);
  std::error_code create(StringRef D, uint64_t BaseAddr) {
    Data = D;
    Address = BaseAddr;
    return std::error_code();
  }

  /// Return function's PGO name from the function name's symabol
  /// address in the object file. If an error occurs, Return
  /// an empty string.
  StringRef getFuncName(uint64_t FuncNameAddress, size_t NameSize);
};

struct InstrProfStringTable {
  // Set of string values in profiling data.
  StringSet<> StringValueSet;
  InstrProfStringTable() { StringValueSet.clear(); }
  // Get a pointer to internal storage of a string in set
  const char *getStringData(StringRef Str) {
    auto Result = StringValueSet.find(Str);
    return (Result == StringValueSet.end()) ? nullptr : Result->first().data();
  }
  // Insert a string to StringTable
  const char *insertString(StringRef Str) {
    auto Result = StringValueSet.insert(Str);
    return Result.first->first().data();
  }
};

struct InstrProfValueSiteRecord {
  /// Value profiling data pairs at a given value site.
  std::list<InstrProfValueData> ValueData;

  InstrProfValueSiteRecord() { ValueData.clear(); }
  template <class InputIterator>
  InstrProfValueSiteRecord(InputIterator F, InputIterator L)
      : ValueData(F, L) {}

  /// Sort ValueData ascending by Value
  void sortByTargetValues() {
    ValueData.sort(
        [](const InstrProfValueData &left, const InstrProfValueData &right) {
          return left.Value < right.Value;
        });
  }

  /// Merge data from another InstrProfValueSiteRecord
  /// Optionally scale merged counts by \p Weight.
  instrprof_error mergeValueData(InstrProfValueSiteRecord &Input,
                                 uint64_t Weight = 1) {
    this->sortByTargetValues();
    Input.sortByTargetValues();
    auto I = ValueData.begin();
    auto IE = ValueData.end();
    instrprof_error Result = instrprof_error::success;
    for (auto J = Input.ValueData.begin(), JE = Input.ValueData.end(); J != JE;
         ++J) {
      while (I != IE && I->Value < J->Value)
        ++I;
      if (I != IE && I->Value == J->Value) {
        uint64_t JCount = J->Count;
        bool Overflowed;
        if (Weight > 1) {
          JCount = SaturatingMultiply(JCount, Weight, &Overflowed);
          if (Overflowed)
            Result = instrprof_error::counter_overflow;
        }
        I->Count = SaturatingAdd(I->Count, JCount, &Overflowed);
        if (Overflowed)
          Result = instrprof_error::counter_overflow;
        ++I;
        continue;
      }
      ValueData.insert(I, *J);
    }
    return Result;
  }
};

/// Profiling information for a single function.
struct InstrProfRecord {
  InstrProfRecord() {}
  InstrProfRecord(StringRef Name, uint64_t Hash, std::vector<uint64_t> Counts)
      : Name(Name), Hash(Hash), Counts(std::move(Counts)) {}
  StringRef Name;
  uint64_t Hash;
  std::vector<uint64_t> Counts;

  typedef std::vector<std::pair<uint64_t, const char *>> ValueMapType;

  /// Return the number of value profile kinds with non-zero number
  /// of profile sites.
  inline uint32_t getNumValueKinds() const;
  /// Return the number of instrumented sites for ValueKind.
  inline uint32_t getNumValueSites(uint32_t ValueKind) const;
  /// Return the total number of ValueData for ValueKind.
  inline uint32_t getNumValueData(uint32_t ValueKind) const;
  /// Return the number of value data collected for ValueKind at profiling
  /// site: Site.
  inline uint32_t getNumValueDataForSite(uint32_t ValueKind,
                                         uint32_t Site) const;
  /// Return the array of profiled values at \p Site.
  inline std::unique_ptr<InstrProfValueData[]>
  getValueForSite(uint32_t ValueKind, uint32_t Site,
                  uint64_t (*ValueMapper)(uint32_t, uint64_t) = 0) const;
  inline void
  getValueForSite(InstrProfValueData Dest[], uint32_t ValueKind, uint32_t Site,
                  uint64_t (*ValueMapper)(uint32_t, uint64_t) = 0) const;
  /// Reserve space for NumValueSites sites.
  inline void reserveSites(uint32_t ValueKind, uint32_t NumValueSites);
  /// Add ValueData for ValueKind at value Site.
  inline void addValueData(uint32_t ValueKind, uint32_t Site,
                           InstrProfValueData *VData, uint32_t N,
                           ValueMapType *HashKeys);

  /// Merge the counts in \p Other into this one.
  /// Optionally scale merged counts by \p Weight.
  inline instrprof_error merge(InstrProfRecord &Other, uint64_t Weight = 1);

  /// Used by InstrProfWriter: update the value strings to commoned strings in
  /// the writer instance.
  inline void updateStrings(InstrProfStringTable *StrTab);

  /// Clear value data entries
  inline void clearValueData() {
    for (uint32_t Kind = IPVK_First; Kind <= IPVK_Last; ++Kind)
      getValueSitesForKind(Kind).clear();
  }

private:
  std::vector<InstrProfValueSiteRecord> IndirectCallSites;
  const std::vector<InstrProfValueSiteRecord> &
  getValueSitesForKind(uint32_t ValueKind) const {
    switch (ValueKind) {
    case IPVK_IndirectCallTarget:
      return IndirectCallSites;
    default:
      llvm_unreachable("Unknown value kind!");
    }
    return IndirectCallSites;
  }

  std::vector<InstrProfValueSiteRecord> &
  getValueSitesForKind(uint32_t ValueKind) {
    return const_cast<std::vector<InstrProfValueSiteRecord> &>(
        const_cast<const InstrProfRecord *>(this)
            ->getValueSitesForKind(ValueKind));
  }

  // Map indirect call target name hash to name string.
  uint64_t remapValue(uint64_t Value, uint32_t ValueKind,
                      ValueMapType *HashKeys) {
    if (!HashKeys)
      return Value;
    switch (ValueKind) {
    case IPVK_IndirectCallTarget: {
      auto Result =
          std::lower_bound(HashKeys->begin(), HashKeys->end(), Value,
                           [](const std::pair<uint64_t, const char *> &LHS,
                              uint64_t RHS) { return LHS.first < RHS; });
      if (Result != HashKeys->end())
        Value = (uint64_t)Result->second;
      break;
    }
    }
    return Value;
  }

  // Merge Value Profile data from Src record to this record for ValueKind.
  // Scale merged value counts by \p Weight.
  instrprof_error mergeValueProfData(uint32_t ValueKind, InstrProfRecord &Src,
                                     uint64_t Weight) {
    uint32_t ThisNumValueSites = getNumValueSites(ValueKind);
    uint32_t OtherNumValueSites = Src.getNumValueSites(ValueKind);
    if (ThisNumValueSites != OtherNumValueSites)
      return instrprof_error::value_site_count_mismatch;
    std::vector<InstrProfValueSiteRecord> &ThisSiteRecords =
        getValueSitesForKind(ValueKind);
    std::vector<InstrProfValueSiteRecord> &OtherSiteRecords =
        Src.getValueSitesForKind(ValueKind);
    instrprof_error Result = instrprof_error::success;
    for (uint32_t I = 0; I < ThisNumValueSites; I++)
      MergeResult(Result, ThisSiteRecords[I].mergeValueData(OtherSiteRecords[I],
                                                            Weight));
    return Result;
  }
};

uint32_t InstrProfRecord::getNumValueKinds() const {
  uint32_t NumValueKinds = 0;
  for (uint32_t Kind = IPVK_First; Kind <= IPVK_Last; ++Kind)
    NumValueKinds += !(getValueSitesForKind(Kind).empty());
  return NumValueKinds;
}

uint32_t InstrProfRecord::getNumValueData(uint32_t ValueKind) const {
  uint32_t N = 0;
  const std::vector<InstrProfValueSiteRecord> &SiteRecords =
      getValueSitesForKind(ValueKind);
  for (auto &SR : SiteRecords) {
    N += SR.ValueData.size();
  }
  return N;
}

uint32_t InstrProfRecord::getNumValueSites(uint32_t ValueKind) const {
  return getValueSitesForKind(ValueKind).size();
}

uint32_t InstrProfRecord::getNumValueDataForSite(uint32_t ValueKind,
                                                 uint32_t Site) const {
  return getValueSitesForKind(ValueKind)[Site].ValueData.size();
}

std::unique_ptr<InstrProfValueData[]> InstrProfRecord::getValueForSite(
    uint32_t ValueKind, uint32_t Site,
    uint64_t (*ValueMapper)(uint32_t, uint64_t)) const {
  uint32_t N = getNumValueDataForSite(ValueKind, Site);
  if (N == 0)
    return std::unique_ptr<InstrProfValueData[]>(nullptr);

  auto VD = llvm::make_unique<InstrProfValueData[]>(N);
  getValueForSite(VD.get(), ValueKind, Site, ValueMapper);

  return VD;
}

void InstrProfRecord::getValueForSite(InstrProfValueData Dest[],
                                      uint32_t ValueKind, uint32_t Site,
                                      uint64_t (*ValueMapper)(uint32_t,
                                                              uint64_t)) const {
  uint32_t I = 0;
  for (auto V : getValueSitesForKind(ValueKind)[Site].ValueData) {
    Dest[I].Value = ValueMapper ? ValueMapper(ValueKind, V.Value) : V.Value;
    Dest[I].Count = V.Count;
    I++;
  }
}

void InstrProfRecord::addValueData(uint32_t ValueKind, uint32_t Site,
                                   InstrProfValueData *VData, uint32_t N,
                                   ValueMapType *HashKeys) {
  for (uint32_t I = 0; I < N; I++) {
    VData[I].Value = remapValue(VData[I].Value, ValueKind, HashKeys);
  }
  std::vector<InstrProfValueSiteRecord> &ValueSites =
      getValueSitesForKind(ValueKind);
  if (N == 0)
    ValueSites.push_back(InstrProfValueSiteRecord());
  else
    ValueSites.emplace_back(VData, VData + N);
}

void InstrProfRecord::reserveSites(uint32_t ValueKind, uint32_t NumValueSites) {
  std::vector<InstrProfValueSiteRecord> &ValueSites =
      getValueSitesForKind(ValueKind);
  ValueSites.reserve(NumValueSites);
}

void InstrProfRecord::updateStrings(InstrProfStringTable *StrTab) {
  if (!StrTab)
    return;

  Name = StrTab->insertString(Name);
  for (auto &VSite : IndirectCallSites)
    for (auto &VData : VSite.ValueData)
      VData.Value = (uint64_t)StrTab->insertString((const char *)VData.Value);
}

instrprof_error InstrProfRecord::merge(InstrProfRecord &Other,
                                       uint64_t Weight) {
  // If the number of counters doesn't match we either have bad data
  // or a hash collision.
  if (Counts.size() != Other.Counts.size())
    return instrprof_error::count_mismatch;

  instrprof_error Result = instrprof_error::success;

  for (size_t I = 0, E = Other.Counts.size(); I < E; ++I) {
    bool Overflowed;
    uint64_t OtherCount = Other.Counts[I];
    if (Weight > 1) {
      OtherCount = SaturatingMultiply(OtherCount, Weight, &Overflowed);
      if (Overflowed)
        Result = instrprof_error::counter_overflow;
    }
    Counts[I] = SaturatingAdd(Counts[I], OtherCount, &Overflowed);
    if (Overflowed)
      Result = instrprof_error::counter_overflow;
  }

  for (uint32_t Kind = IPVK_First; Kind <= IPVK_Last; ++Kind)
    MergeResult(Result, mergeValueProfData(Kind, Other, Weight));

  return Result;
}

inline support::endianness getHostEndianness() {
  return sys::IsLittleEndianHost ? support::little : support::big;
}


// Include definitions for value profile data
#define INSTR_PROF_VALUE_PROF_DATA
#include "llvm/ProfileData/InstrProfData.inc"

 /*
 * Initialize the record for runtime value profile data.
 * Return 0 if the initialization is successful, otherwise
 * return 1.
 */
int initializeValueProfRuntimeRecord(ValueProfRuntimeRecord *RuntimeRecord,
                                     const uint16_t *NumValueSites,
                                     ValueProfNode **Nodes);

/* Release memory allocated for the runtime record.  */
void finalizeValueProfRuntimeRecord(ValueProfRuntimeRecord *RuntimeRecord);

/* Return the size of ValueProfData structure that can be used to store
   the value profile data collected at runtime. */
uint32_t getValueProfDataSizeRT(const ValueProfRuntimeRecord *Record);

/* Return a ValueProfData instance that stores the data collected at runtime. */
ValueProfData *
serializeValueProfDataFromRT(const ValueProfRuntimeRecord *Record,
                             ValueProfData *Dst);

namespace IndexedInstrProf {

enum class HashT : uint32_t {
  MD5,

  Last = MD5
};

static inline uint64_t MD5Hash(StringRef Str) {
  MD5 Hash;
  Hash.update(Str);
  llvm::MD5::MD5Result Result;
  Hash.final(Result);
  // Return the least significant 8 bytes. Our MD5 implementation returns the
  // result in little endian, so we may need to swap bytes.
  using namespace llvm::support;
  return endian::read<uint64_t, little, unaligned>(Result);
}

static inline uint64_t ComputeHash(HashT Type, StringRef K) {
  switch (Type) {
  case HashT::MD5:
    return IndexedInstrProf::MD5Hash(K);
  }
  llvm_unreachable("Unhandled hash type");
}

const uint64_t Magic = 0x8169666f72706cff; // "\xfflprofi\x81"
const uint64_t Version = INSTR_PROF_INDEX_VERSION;
const HashT HashType = HashT::MD5;

static inline uint64_t ComputeHash(StringRef K) {
  return ComputeHash(HashType, K);
}

// This structure defines the file header of the LLVM profile
// data file in indexed-format.
struct Header {
  uint64_t Magic;
  uint64_t Version;
  uint64_t MaxFunctionCount;
  uint64_t HashType;
  uint64_t HashOffset;
};

} // end namespace IndexedInstrProf

namespace RawInstrProf {

const uint64_t Version = INSTR_PROF_RAW_VERSION;

template <class IntPtrT> inline uint64_t getMagic();
template <> inline uint64_t getMagic<uint64_t>() {
  return INSTR_PROF_RAW_MAGIC_64;
}

template <> inline uint64_t getMagic<uint32_t>() {
  return INSTR_PROF_RAW_MAGIC_32;
}

// Per-function profile data header/control structure.
// The definition should match the structure defined in
// compiler-rt/lib/profile/InstrProfiling.h.
// It should also match the synthesized type in
// Transforms/Instrumentation/InstrProfiling.cpp:getOrCreateRegionCounters.
template <class IntPtrT> struct LLVM_ALIGNAS(8) ProfileData {
  #define INSTR_PROF_DATA(Type, LLVMType, Name, Init) Type Name;
  #include "llvm/ProfileData/InstrProfData.inc"
};

// File header structure of the LLVM profile data in raw format.
// The definition should match the header referenced in
// compiler-rt/lib/profile/InstrProfilingFile.c  and
// InstrProfilingBuffer.c.
struct Header {
#define INSTR_PROF_RAW_HEADER(Type, Name, Init) const Type Name;
#include "llvm/ProfileData/InstrProfData.inc"
};

}  // end namespace RawInstrProf

namespace coverage {

// Profile coverage map has the following layout:
// [CoverageMapFileHeader]
// [ArrayStart]
//  [CovMapFunctionRecord]
//  [CovMapFunctionRecord]
//  ...
// [ArrayEnd]
// [Encoded Region Mapping Data]
LLVM_PACKED_START
template <class IntPtrT> struct CovMapFunctionRecord {
  #define COVMAP_FUNC_RECORD(Type, LLVMType, Name, Init) Type Name;
  #include "llvm/ProfileData/InstrProfData.inc"
};
LLVM_PACKED_END

}

} // end namespace llvm

namespace std {
template <>
struct is_error_code_enum<llvm::instrprof_error> : std::true_type {};
}

#endif // LLVM_PROFILEDATA_INSTRPROF_H_
