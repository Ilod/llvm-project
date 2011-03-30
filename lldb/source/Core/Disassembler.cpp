//===-- Disassembler.cpp ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/Disassembler.h"

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/lldb-private.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Timer.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"

#define DEFAULT_DISASM_BYTE_SIZE 32

using namespace lldb;
using namespace lldb_private;


Disassembler*
Disassembler::FindPlugin (const ArchSpec &arch, const char *plugin_name)
{
    Timer scoped_timer (__PRETTY_FUNCTION__,
                        "Disassembler::FindPlugin (arch = %s, plugin_name = %s)",
                        arch.GetArchitectureName(),
                        plugin_name);

    std::auto_ptr<Disassembler> disassembler_ap;
    DisassemblerCreateInstance create_callback = NULL;
    
    if (plugin_name)
    {
        create_callback = PluginManager::GetDisassemblerCreateCallbackForPluginName (plugin_name);
        if (create_callback)
        {
            disassembler_ap.reset (create_callback(arch));
            
            if (disassembler_ap.get())
                return disassembler_ap.release();
        }
    }
    else
    {
        for (uint32_t idx = 0; (create_callback = PluginManager::GetDisassemblerCreateCallbackAtIndex(idx)) != NULL; ++idx)
        {
            disassembler_ap.reset (create_callback(arch));

            if (disassembler_ap.get())
                return disassembler_ap.release();
        }
    }
    return NULL;
}


static void
ResolveAddress (const ExecutionContext &exe_ctx,
                const Address &addr, 
                Address &resolved_addr)
{
    if (!addr.IsSectionOffset())
    {
        // If we weren't passed in a section offset address range,
        // try and resolve it to something
        if (exe_ctx.target)
        {
            if (exe_ctx.target->GetSectionLoadList().IsEmpty())
            {
                exe_ctx.target->GetImages().ResolveFileAddress (addr.GetOffset(), resolved_addr);
            }
            else
            {
                exe_ctx.target->GetSectionLoadList().ResolveLoadAddress (addr.GetOffset(), resolved_addr);
            }
            // We weren't able to resolve the address, just treat it as a
            // raw address
            if (resolved_addr.IsValid())
                return;
        }
    }
    resolved_addr = addr;
}

size_t
Disassembler::Disassemble
(
    Debugger &debugger,
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    SymbolContextList &sc_list,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    size_t success_count = 0;
    const size_t count = sc_list.GetSize();
    SymbolContext sc;
    AddressRange range;
    
    for (size_t i=0; i<count; ++i)
    {
        if (sc_list.GetContextAtIndex(i, sc) == false)
            break;
        if (sc.GetAddressRange(eSymbolContextFunction | eSymbolContextSymbol, range))
        {
            if (Disassemble (debugger, 
                             arch, 
                             plugin_name,
                             exe_ctx, 
                             range, 
                             num_instructions,
                             num_mixed_context_lines, 
                             show_bytes, 
                             raw, 
                             strm))
            {
                ++success_count;
                strm.EOL();
            }
        }
    }
    return success_count;
}

bool
Disassembler::Disassemble
(
    Debugger &debugger,
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    const ConstString &name,
    Module *module,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    SymbolContextList sc_list;
    if (name)
    {
        const bool include_symbols = true;
        if (module)
        {
            module->FindFunctions (name, 
                                   eFunctionNameTypeBase | 
                                   eFunctionNameTypeFull | 
                                   eFunctionNameTypeMethod | 
                                   eFunctionNameTypeSelector, 
                                   include_symbols,
                                   true,
                                   sc_list);
        }
        else if (exe_ctx.target)
        {
            exe_ctx.target->GetImages().FindFunctions (name, 
                                                       eFunctionNameTypeBase | 
                                                       eFunctionNameTypeFull | 
                                                       eFunctionNameTypeMethod | 
                                                       eFunctionNameTypeSelector,
                                                       include_symbols, 
                                                       false,
                                                       sc_list);
        }
    }
    
    if (sc_list.GetSize ())
    {
        return Disassemble (debugger, 
                            arch, 
                            plugin_name,
                            exe_ctx, 
                            sc_list,
                            num_instructions, 
                            num_mixed_context_lines, 
                            show_bytes,
                            raw,
                            strm);
    }
    return false;
}


lldb::DisassemblerSP
Disassembler::DisassembleRange
(
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    const AddressRange &range
)
{
    lldb::DisassemblerSP disasm_sp;
    if (range.GetByteSize() > 0 && range.GetBaseAddress().IsValid())
    {
        disasm_sp.reset (Disassembler::FindPlugin(arch, plugin_name));

        if (disasm_sp)
        {
            size_t bytes_disassembled = disasm_sp->ParseInstructions (&exe_ctx, range);
            if (bytes_disassembled == 0)
                disasm_sp.reset();
        }
    }
    return disasm_sp;
}


bool
Disassembler::Disassemble
(
    Debugger &debugger,
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    const AddressRange &disasm_range,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    if (disasm_range.GetByteSize())
    {
        std::auto_ptr<Disassembler> disasm_ap (Disassembler::FindPlugin(arch, plugin_name));

        if (disasm_ap.get())
        {
            AddressRange range;
            ResolveAddress (exe_ctx, disasm_range.GetBaseAddress(), range.GetBaseAddress());
            range.SetByteSize (disasm_range.GetByteSize());
            
            size_t bytes_disassembled = disasm_ap->ParseInstructions (&exe_ctx, range);
            if (bytes_disassembled == 0)
                return false;

            return PrintInstructions (disasm_ap.get(),
                                      debugger,
                                      arch,
                                      exe_ctx,
                                      num_instructions,
                                      num_mixed_context_lines,
                                      show_bytes,
                                      raw,
                                      strm);
        }
    }
    return false;
}
            
bool
Disassembler::Disassemble
(
    Debugger &debugger,
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    const Address &start_address,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    if (num_instructions > 0)
    {
        std::auto_ptr<Disassembler> disasm_ap (Disassembler::FindPlugin(arch, plugin_name));
        if (disasm_ap.get())
        {
            Address addr;
            ResolveAddress (exe_ctx, start_address, addr);

            size_t bytes_disassembled = disasm_ap->ParseInstructions (&exe_ctx, addr, num_instructions);
            if (bytes_disassembled == 0)
                return false;
            return PrintInstructions (disasm_ap.get(),
                                      debugger,
                                      arch,
                                      exe_ctx,
                                      num_instructions,
                                      num_mixed_context_lines,
                                      show_bytes,
                                      raw,
                                      strm);
        }
    }
    return false;
}
            
bool 
Disassembler::PrintInstructions
(
    Disassembler *disasm_ptr,
    Debugger &debugger,
    const ArchSpec &arch,
    const ExecutionContext &exe_ctx,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    // We got some things disassembled...
    size_t num_instructions_found = disasm_ptr->GetInstructionList().GetSize();
    
    if (num_instructions > 0 && num_instructions < num_instructions_found)
        num_instructions_found = num_instructions;
        
    const uint32_t max_opcode_byte_size = disasm_ptr->GetInstructionList().GetMaxOpcocdeByteSize ();
    uint32_t offset = 0;
    SymbolContext sc;
    SymbolContext prev_sc;
    AddressRange sc_range;
    Address *pc_addr_ptr = NULL;
    if (exe_ctx.frame)
        pc_addr_ptr = &exe_ctx.frame->GetFrameCodeAddress();

    for (size_t i=0; i<num_instructions_found; ++i)
    {
        Instruction *inst = disasm_ptr->GetInstructionList().GetInstructionAtIndex (i).get();
        if (inst)
        {
            const Address &addr = inst->GetAddress();
            const bool inst_is_at_pc = pc_addr_ptr && addr == *pc_addr_ptr;

            prev_sc = sc;

            Module *module = addr.GetModule();
            if (module)
            {
                uint32_t resolved_mask = module->ResolveSymbolContextForAddress(addr, eSymbolContextEverything, sc);
                if (resolved_mask)
                {
                    if (num_mixed_context_lines)
                    {
                        if (!sc_range.ContainsFileAddress (addr))
                        {
                            sc.GetAddressRange (eSymbolContextEverything, sc_range);
                            
                            if (sc != prev_sc)
                            {
                                if (offset != 0)
                                    strm.EOL();
                                
                                sc.DumpStopContext(&strm, exe_ctx.process, addr, false, true, false);
                                strm.EOL();
                                
                                if (sc.comp_unit && sc.line_entry.IsValid())
                                {
                                    debugger.GetSourceManager().DisplaySourceLinesWithLineNumbers (sc.line_entry.file,
                                                                                                   sc.line_entry.line,
                                                                                                   num_mixed_context_lines,
                                                                                                   num_mixed_context_lines,
                                                                                                   num_mixed_context_lines ? "->" : "",
                                                                                                   &strm);
                                }
                            }
                        }
                    }
                    else if (!(prev_sc.function == sc.function || prev_sc.symbol == sc.symbol))
                    {
                        if (prev_sc.function || prev_sc.symbol)
                            strm.EOL();

                        strm << sc.module_sp->GetFileSpec().GetFilename();
                        
                        if (sc.function)
                            strm << '`' << sc.function->GetMangled().GetName();
                        else if (sc.symbol)
                            strm << '`' << sc.symbol->GetMangled().GetName();
                        strm << ":\n";
                    }
                }
                else
                {
                    sc.Clear();
                }
            }

            if (pc_addr_ptr)
            {
                if (inst_is_at_pc)
                    strm.PutCString("-> ");
                else
                    strm.PutCString("   ");
            }
            inst->Dump(&strm, max_opcode_byte_size, true, show_bytes, &exe_ctx, raw);
            strm.EOL();            
        }
        else
        {
            break;
        }
    }
        
    return true;
}


bool
Disassembler::Disassemble
(
    Debugger &debugger,
    const ArchSpec &arch,
    const char *plugin_name,
    const ExecutionContext &exe_ctx,
    uint32_t num_instructions,
    uint32_t num_mixed_context_lines,
    bool show_bytes,
    bool raw,
    Stream &strm
)
{
    AddressRange range;
    if (exe_ctx.frame)
    {
        SymbolContext sc(exe_ctx.frame->GetSymbolContext(eSymbolContextFunction | eSymbolContextSymbol));
        if (sc.function)
        {
            range = sc.function->GetAddressRange();
        }
        else if (sc.symbol && sc.symbol->GetAddressRangePtr())
        {
            range = *sc.symbol->GetAddressRangePtr();
        }
        else
        {
            range.GetBaseAddress() = exe_ctx.frame->GetFrameCodeAddress();
        }

        if (range.GetBaseAddress().IsValid() && range.GetByteSize() == 0)
            range.SetByteSize (DEFAULT_DISASM_BYTE_SIZE);
    }

    return Disassemble (debugger, 
                        arch, 
                        plugin_name,
                        exe_ctx, 
                        range, 
                        num_instructions, 
                        num_mixed_context_lines, 
                        show_bytes, 
                        raw, 
                        strm);
}

Instruction::Instruction(const Address &address, AddressClass addr_class) :
    m_address (address),
    m_address_class (addr_class),
    m_opcode()
{
}

Instruction::~Instruction()
{
}

AddressClass
Instruction::GetAddressClass ()
{
    if (m_address_class == eAddressClassInvalid)
        m_address_class = m_address.GetAddressClass();
    return m_address_class;
}

InstructionList::InstructionList() :
    m_instructions()
{
}

InstructionList::~InstructionList()
{
}

size_t
InstructionList::GetSize() const
{
    return m_instructions.size();
}

uint32_t
InstructionList::GetMaxOpcocdeByteSize () const
{
    uint32_t max_inst_size = 0;
    collection::const_iterator pos, end;
    for (pos = m_instructions.begin(), end = m_instructions.end();
         pos != end;
         ++pos)
    {
        uint32_t inst_size = (*pos)->GetOpcode().GetByteSize();
        if (max_inst_size < inst_size)
            max_inst_size = inst_size;
    }
    return max_inst_size;
}



InstructionSP
InstructionList::GetInstructionAtIndex (uint32_t idx) const
{
    InstructionSP inst_sp;
    if (idx < m_instructions.size())
        inst_sp = m_instructions[idx];
    return inst_sp;
}

void
InstructionList::Clear()
{
  m_instructions.clear();
}

void
InstructionList::Append (lldb::InstructionSP &inst_sp)
{
    if (inst_sp)
        m_instructions.push_back(inst_sp);
}


size_t
Disassembler::ParseInstructions
(
    const ExecutionContext *exe_ctx,
    const AddressRange &range
)
{
    Target *target = exe_ctx->target;
    const addr_t byte_size = range.GetByteSize();
    if (target == NULL || byte_size == 0 || !range.GetBaseAddress().IsValid())
        return 0;

    DataBufferHeap *heap_buffer = new DataBufferHeap (byte_size, '\0');
    DataBufferSP data_sp(heap_buffer);

    Error error;
    const bool prefer_file_cache = true;
    const size_t bytes_read = target->ReadMemory (range.GetBaseAddress(), 
                                                  prefer_file_cache, 
                                                  heap_buffer->GetBytes(), 
                                                  heap_buffer->GetByteSize(), 
                                                  error);
    
    if (bytes_read > 0)
    {
        if (bytes_read != heap_buffer->GetByteSize())
            heap_buffer->SetByteSize (bytes_read);
        DataExtractor data (data_sp, 
                            m_arch.GetByteOrder(),
                            m_arch.GetAddressByteSize());
        return DecodeInstructions (range.GetBaseAddress(), data, 0, UINT32_MAX, false);
    }

    return 0;
}

size_t
Disassembler::ParseInstructions
(
    const ExecutionContext *exe_ctx,
    const Address &start,
    uint32_t num_instructions
)
{
    m_instruction_list.Clear();

    if (num_instructions == 0 || !start.IsValid())
        return 0;
        
    Target *target = exe_ctx->target;
    // Calculate the max buffer size we will need in order to disassemble
    const addr_t byte_size = num_instructions * m_arch.GetMaximumOpcodeByteSize();
    
    if (target == NULL || byte_size == 0)
        return 0;

    DataBufferHeap *heap_buffer = new DataBufferHeap (byte_size, '\0');
    DataBufferSP data_sp (heap_buffer);

    Error error;
    bool prefer_file_cache = true;
    const size_t bytes_read = target->ReadMemory (start, 
                                                  prefer_file_cache, 
                                                  heap_buffer->GetBytes(), 
                                                  byte_size, 
                                                  error);

    if (bytes_read == 0)
        return 0;
    DataExtractor data (data_sp,
                        m_arch.GetByteOrder(),
                        m_arch.GetAddressByteSize());

    const bool append_instructions = true;
    DecodeInstructions (start, 
                        data, 
                        0, 
                        num_instructions, 
                        append_instructions);

    return m_instruction_list.GetSize();
}

//----------------------------------------------------------------------
// Disassembler copy constructor
//----------------------------------------------------------------------
Disassembler::Disassembler(const ArchSpec& arch) :
    m_arch (arch),
    m_instruction_list(),
    m_base_addr(LLDB_INVALID_ADDRESS)
{

}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
Disassembler::~Disassembler()
{
}

InstructionList &
Disassembler::GetInstructionList ()
{
    return m_instruction_list;
}

const InstructionList &
Disassembler::GetInstructionList () const
{
    return m_instruction_list;
}
