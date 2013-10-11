//===-- BreakpointLocation.cpp ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

// C Includes
// C++ Includes
#include <string>

// Other libraries and framework includes
// Project includes
#include "lldb/lldb-private-log.h"
#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Breakpoint/BreakpointID.h"
#include "lldb/Breakpoint/StoppointCallbackContext.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Symbol/CompileUnit.h"
#include "lldb/Symbol/Symbol.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/ThreadSpec.h"

using namespace lldb;
using namespace lldb_private;

BreakpointLocation::BreakpointLocation
(
    break_id_t loc_id,
    Breakpoint &owner,
    const Address &addr,
    lldb::tid_t tid,
    bool hardware
) :
    StoppointLocation (loc_id, addr.GetOpcodeLoadAddress(&owner.GetTarget()), hardware),
    m_being_created(true),
    m_address (addr),
    m_owner (owner),
    m_options_ap (),
    m_bp_site_sp (),
    m_condition_mutex ()
{
    SetThreadID (tid);
    m_being_created = false;
}

BreakpointLocation::~BreakpointLocation()
{
    ClearBreakpointSite();
}

lldb::addr_t
BreakpointLocation::GetLoadAddress () const
{
    return m_address.GetOpcodeLoadAddress (&m_owner.GetTarget());
}

Address &
BreakpointLocation::GetAddress ()
{
    return m_address;
}

Breakpoint &
BreakpointLocation::GetBreakpoint ()
{
    return m_owner;
}

bool
BreakpointLocation::IsEnabled () const
{
    if (!m_owner.IsEnabled())
        return false;
    else if (m_options_ap.get() != NULL)
        return m_options_ap->IsEnabled();
    else
        return true;
}

void
BreakpointLocation::SetEnabled (bool enabled)
{
    GetLocationOptions()->SetEnabled(enabled);
    if (enabled)
    {
        ResolveBreakpointSite();
    }
    else
    {
        ClearBreakpointSite();
    }
    SendBreakpointLocationChangedEvent (enabled ? eBreakpointEventTypeEnabled : eBreakpointEventTypeDisabled);
}

void
BreakpointLocation::SetThreadID (lldb::tid_t thread_id)
{
    if (thread_id != LLDB_INVALID_THREAD_ID)
        GetLocationOptions()->SetThreadID(thread_id);
    else
    {
        // If we're resetting this to an invalid thread id, then
        // don't make an options pointer just to do that.
        if (m_options_ap.get() != NULL)
            m_options_ap->SetThreadID (thread_id);
    }
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeThreadChanged);
}

lldb::tid_t
BreakpointLocation::GetThreadID ()
{
    if (GetOptionsNoCreate()->GetThreadSpecNoCreate())
        return GetOptionsNoCreate()->GetThreadSpecNoCreate()->GetTID();
    else
        return LLDB_INVALID_THREAD_ID;
}

void
BreakpointLocation::SetThreadIndex (uint32_t index)
{
    if (index != 0)
        GetLocationOptions()->GetThreadSpec()->SetIndex(index);
    else
    {
        // If we're resetting this to an invalid thread id, then
        // don't make an options pointer just to do that.
        if (m_options_ap.get() != NULL)
            m_options_ap->GetThreadSpec()->SetIndex(index);
    }
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeThreadChanged);
                        
}

uint32_t
BreakpointLocation::GetThreadIndex() const
{
    if (GetOptionsNoCreate()->GetThreadSpecNoCreate())
        return GetOptionsNoCreate()->GetThreadSpecNoCreate()->GetIndex();
    else
        return 0;
}

void
BreakpointLocation::SetThreadName (const char *thread_name)
{
    if (thread_name != NULL)
        GetLocationOptions()->GetThreadSpec()->SetName(thread_name);
    else
    {
        // If we're resetting this to an invalid thread id, then
        // don't make an options pointer just to do that.
        if (m_options_ap.get() != NULL)
            m_options_ap->GetThreadSpec()->SetName(thread_name);
    }
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeThreadChanged);
}

const char *
BreakpointLocation::GetThreadName () const
{
    if (GetOptionsNoCreate()->GetThreadSpecNoCreate())
        return GetOptionsNoCreate()->GetThreadSpecNoCreate()->GetName();
    else
        return NULL;
}

void 
BreakpointLocation::SetQueueName (const char *queue_name)
{
    if (queue_name != NULL)
        GetLocationOptions()->GetThreadSpec()->SetQueueName(queue_name);
    else
    {
        // If we're resetting this to an invalid thread id, then
        // don't make an options pointer just to do that.
        if (m_options_ap.get() != NULL)
            m_options_ap->GetThreadSpec()->SetQueueName(queue_name);
    }
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeThreadChanged);
}

const char *
BreakpointLocation::GetQueueName () const
{
    if (GetOptionsNoCreate()->GetThreadSpecNoCreate())
        return GetOptionsNoCreate()->GetThreadSpecNoCreate()->GetQueueName();
    else
        return NULL;
}

bool
BreakpointLocation::InvokeCallback (StoppointCallbackContext *context)
{
    if (m_options_ap.get() != NULL && m_options_ap->HasCallback())
        return m_options_ap->InvokeCallback (context, m_owner.GetID(), GetID());
    else    
        return m_owner.InvokeCallback (context, GetID());
}

void
BreakpointLocation::SetCallback (BreakpointHitCallback callback, void *baton,
                 bool is_synchronous)
{
    // The default "Baton" class will keep a copy of "baton" and won't free
    // or delete it when it goes goes out of scope.
    GetLocationOptions()->SetCallback(callback, BatonSP (new Baton(baton)), is_synchronous);
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeCommandChanged);
}

void
BreakpointLocation::SetCallback (BreakpointHitCallback callback, const BatonSP &baton_sp,
                 bool is_synchronous)
{
    GetLocationOptions()->SetCallback (callback, baton_sp, is_synchronous);
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeCommandChanged);
}


void
BreakpointLocation::ClearCallback ()
{
    GetLocationOptions()->ClearCallback();
}

void 
BreakpointLocation::SetCondition (const char *condition)
{
    GetLocationOptions()->SetCondition (condition);
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeConditionChanged);
}

const char *
BreakpointLocation::GetConditionText (size_t *hash) const
{
    return GetOptionsNoCreate()->GetConditionText(hash);
}

bool
BreakpointLocation::ConditionSaysStop (ExecutionContext &exe_ctx, Error &error)
{
    Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_BREAKPOINTS);
 
    Mutex::Locker evaluation_locker(m_condition_mutex);
    
    size_t condition_hash;
    const char *condition_text = GetConditionText(&condition_hash);
    
    if (!condition_text)
    {
        m_user_expression_sp.reset();
        return false;
    }
    
    if (condition_hash != m_condition_hash ||
        !m_user_expression_sp ||
        !m_user_expression_sp->MatchesContext(exe_ctx))
    {
        m_user_expression_sp.reset(new ClangUserExpression(condition_text,
                                                           NULL,
                                                           lldb::eLanguageTypeUnknown,
                                                           ClangUserExpression::eResultTypeAny));
        
        StreamString errors;
        
        if (!m_user_expression_sp->Parse(errors,
                                         exe_ctx,
                                         eExecutionPolicyOnlyWhenNeeded,
                                         true))
        {
            error.SetErrorStringWithFormat("Couldn't parse conditional expression:\n%s",
                                           errors.GetData());
            m_user_expression_sp.reset();
            return false;
        }
        
        m_condition_hash = condition_hash;
    }

    // We need to make sure the user sees any parse errors in their condition, so we'll hook the
    // constructor errors up to the debugger's Async I/O.
        
    ValueObjectSP result_value_sp;
    const bool unwind_on_error = true;
    const bool ignore_breakpoints = true;
    const bool try_all_threads = true;
    
    Error expr_error;
    
    StreamString execution_errors;
    
    ClangExpressionVariableSP result_variable_sp;
    
    ExecutionResults result_code =
    m_user_expression_sp->Execute(execution_errors,
                                  exe_ctx,
                                  unwind_on_error,
                                  ignore_breakpoints,
                                  m_user_expression_sp,
                                  result_variable_sp,
                                  try_all_threads,
                                  ClangUserExpression::kDefaultTimeout);
    
    bool ret;
    
    if (result_code == eExecutionCompleted)
    {
        if (!result_variable_sp)
        {
            ret = false;
            error.SetErrorString("Expression did not return a result");
            return false;
        }
        
        result_value_sp = result_variable_sp->GetValueObject();

        if (result_value_sp)
        {
            Scalar scalar_value;
            if (result_value_sp->ResolveValue (scalar_value))
            {
                if (scalar_value.ULongLong(1) == 0)
                    ret = false;
                else
                    ret = true;
                if (log)
                    log->Printf("Condition successfully evaluated, result is %s.\n",
                                ret ? "true" : "false");
            }
            else
            {
                ret = false;
                error.SetErrorString("Failed to get an integer result from the expression");
            }
        }
        else
        {
            ret = false;
            error.SetErrorString("Failed to get any result from the expression");
        }
    }
    else
    {
        ret = false;
        error.SetErrorStringWithFormat("Couldn't execute expression:\n%s", execution_errors.GetData());
    }
    
    return ret;
}

uint32_t
BreakpointLocation::GetIgnoreCount ()
{
    return GetOptionsNoCreate()->GetIgnoreCount();
}

void
BreakpointLocation::SetIgnoreCount (uint32_t n)
{
    GetLocationOptions()->SetIgnoreCount(n);
    SendBreakpointLocationChangedEvent (eBreakpointEventTypeIgnoreChanged);
}

void
BreakpointLocation::DecrementIgnoreCount()
{
    if (m_options_ap.get() != NULL)
    {
        uint32_t loc_ignore = m_options_ap->GetIgnoreCount();
        if (loc_ignore != 0)
            m_options_ap->SetIgnoreCount(loc_ignore - 1);
    }
}

bool
BreakpointLocation::IgnoreCountShouldStop()
{
    if (m_options_ap.get() != NULL)
    {
        uint32_t loc_ignore = m_options_ap->GetIgnoreCount();
        if (loc_ignore != 0)
        {
            m_owner.DecrementIgnoreCount();
            DecrementIgnoreCount();          // Have to decrement our owners' ignore count, since it won't get a
                                             // chance to.
            return false;
        }
    }
    return true;
}

const BreakpointOptions *
BreakpointLocation::GetOptionsNoCreate () const
{
    if (m_options_ap.get() != NULL)
        return m_options_ap.get();
    else
        return m_owner.GetOptions ();
}

BreakpointOptions *
BreakpointLocation::GetLocationOptions ()
{
    // If we make the copy we don't copy the callbacks because that is potentially 
    // expensive and we don't want to do that for the simple case where someone is
    // just disabling the location.
    if (m_options_ap.get() == NULL)
        m_options_ap.reset(BreakpointOptions::CopyOptionsNoCallback(*m_owner.GetOptions ()));
    
    return m_options_ap.get();
}

bool
BreakpointLocation::ValidForThisThread (Thread *thread)
{
    return thread->MatchesSpec(GetOptionsNoCreate()->GetThreadSpecNoCreate());
}

// RETURNS - true if we should stop at this breakpoint, false if we
// should continue.  Note, we don't check the thread spec for the breakpoint
// here, since if the breakpoint is not for this thread, then the event won't
// even get reported, so the check is redundant.

bool
BreakpointLocation::ShouldStop (StoppointCallbackContext *context)
{
    bool should_stop = true;
    Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_BREAKPOINTS);

    IncrementHitCount();

    if (!IsEnabled())
        return false;

    if (!IgnoreCountShouldStop())
        return false;
    
    if (!m_owner.IgnoreCountShouldStop())
        return false;

    // We only run synchronous callbacks in ShouldStop:
    context->is_synchronous = true;
    should_stop = InvokeCallback (context);
    
    if (log)
    {
        StreamString s;
        GetDescription (&s, lldb::eDescriptionLevelVerbose);
        log->Printf ("Hit breakpoint location: %s, %s.\n", s.GetData(), should_stop ? "stopping" : "continuing");
    }
    
    return should_stop;
}

bool
BreakpointLocation::IsResolved () const
{
    return m_bp_site_sp.get() != NULL;
}

lldb::BreakpointSiteSP
BreakpointLocation::GetBreakpointSite() const
{
    return m_bp_site_sp;
}

bool
BreakpointLocation::ResolveBreakpointSite ()
{
    if (m_bp_site_sp)
        return true;

    Process *process = m_owner.GetTarget().GetProcessSP().get();
    if (process == NULL)
        return false;

    lldb::break_id_t new_id = process->CreateBreakpointSite (shared_from_this(), m_owner.IsHardware());

    if (new_id == LLDB_INVALID_BREAK_ID)
    {
        Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_BREAKPOINTS);
        if (log)
            log->Warning ("Tried to add breakpoint site at 0x%" PRIx64 " but it was already present.\n",
                          m_address.GetOpcodeLoadAddress (&m_owner.GetTarget()));
        return false;
    }

    return true;
}

bool
BreakpointLocation::SetBreakpointSite (BreakpointSiteSP& bp_site_sp)
{
    m_bp_site_sp = bp_site_sp;
    return true;
}

bool
BreakpointLocation::ClearBreakpointSite ()
{
    if (m_bp_site_sp.get())
    {
        m_owner.GetTarget().GetProcessSP()->RemoveOwnerFromBreakpointSite (GetBreakpoint().GetID(), 
                                                                           GetID(), m_bp_site_sp);
        m_bp_site_sp.reset();
        return true;
    }
    return false;
}

void
BreakpointLocation::GetDescription (Stream *s, lldb::DescriptionLevel level)
{
    SymbolContext sc;
    
    // If the description level is "initial" then the breakpoint is printing out our initial state,
    // and we should let it decide how it wants to print our label.
    if (level != eDescriptionLevelInitial)
    {
        s->Indent();
        BreakpointID::GetCanonicalReference(s, m_owner.GetID(), GetID());
    }
    
    if (level == lldb::eDescriptionLevelBrief)
        return;

    if (level != eDescriptionLevelInitial)
        s->PutCString(": ");

    if (level == lldb::eDescriptionLevelVerbose)
        s->IndentMore();

    if (m_address.IsSectionOffset())
    {
        m_address.CalculateSymbolContext(&sc);

        if (level == lldb::eDescriptionLevelFull || level == eDescriptionLevelInitial)
        {
            s->PutCString("where = ");
            sc.DumpStopContext (s, m_owner.GetTarget().GetProcessSP().get(), m_address, false, true, false);
        }
        else
        {
            if (sc.module_sp)
            {
                s->EOL();
                s->Indent("module = ");
                sc.module_sp->GetFileSpec().Dump (s);
            }

            if (sc.comp_unit != NULL)
            {
                s->EOL();
                s->Indent("compile unit = ");
                static_cast<FileSpec*>(sc.comp_unit)->GetFilename().Dump (s);

                if (sc.function != NULL)
                {
                    s->EOL();
                    s->Indent("function = ");
                    s->PutCString (sc.function->GetMangled().GetName().AsCString("<unknown>"));
                }

                if (sc.line_entry.line > 0)
                {
                    s->EOL();
                    s->Indent("location = ");
                    sc.line_entry.DumpStopContext (s, true);
                }

            }
            else
            {
                // If we don't have a comp unit, see if we have a symbol we can print.
                if (sc.symbol)
                {
                    s->EOL();
                    s->Indent("symbol = ");
                    s->PutCString(sc.symbol->GetMangled().GetName().AsCString("<unknown>"));
                }
            }
        }
    }

    if (level == lldb::eDescriptionLevelVerbose)
    {
        s->EOL();
        s->Indent();
    }
    
    if (m_address.IsSectionOffset() && (level == eDescriptionLevelFull || level == eDescriptionLevelInitial))
        s->Printf (", ");
    s->Printf ("address = ");
    
    ExecutionContextScope *exe_scope = NULL;
    Target *target = &m_owner.GetTarget();
    if (target)
        exe_scope = target->GetProcessSP().get();
    if (exe_scope == NULL)
        exe_scope = target;

    if (eDescriptionLevelInitial)
        m_address.Dump(s, exe_scope, Address::DumpStyleLoadAddress, Address::DumpStyleFileAddress);
    else
        m_address.Dump(s, exe_scope, Address::DumpStyleLoadAddress, Address::DumpStyleModuleWithFileAddress);

    if (level == lldb::eDescriptionLevelVerbose)
    {
        s->EOL();
        s->Indent();
        s->Printf("resolved = %s\n", IsResolved() ? "true" : "false");

        s->Indent();
        s->Printf ("hit count = %-4u\n", GetHitCount());

        if (m_options_ap.get())
        {
            s->Indent();
            m_options_ap->GetDescription (s, level);
            s->EOL();
        }
        s->IndentLess();
    }
    else if (level != eDescriptionLevelInitial)
    {
        s->Printf(", %sresolved, hit count = %u ",
                  (IsResolved() ? "" : "un"),
                  GetHitCount());
        if (m_options_ap.get())
        {
            m_options_ap->GetDescription (s, level);
        }
    }
}

void
BreakpointLocation::Dump(Stream *s) const
{
    if (s == NULL)
        return;

    s->Printf("BreakpointLocation %u: tid = %4.4" PRIx64 "  load addr = 0x%8.8" PRIx64 "  state = %s  type = %s breakpoint  "
              "hw_index = %i  hit_count = %-4u  ignore_count = %-4u",
              GetID(),
              GetOptionsNoCreate()->GetThreadSpecNoCreate()->GetTID(),
              (uint64_t) m_address.GetOpcodeLoadAddress (&m_owner.GetTarget()),
              (m_options_ap.get() ? m_options_ap->IsEnabled() : m_owner.IsEnabled()) ? "enabled " : "disabled",
              IsHardware() ? "hardware" : "software",
              GetHardwareIndex(),
              GetHitCount(),
              GetOptionsNoCreate()->GetIgnoreCount());
}

void
BreakpointLocation::SendBreakpointLocationChangedEvent (lldb::BreakpointEventType eventKind)
{
    if (!m_being_created
        && !m_owner.IsInternal() 
        && m_owner.GetTarget().EventTypeHasListeners(Target::eBroadcastBitBreakpointChanged))
    {
        Breakpoint::BreakpointEventData *data = new Breakpoint::BreakpointEventData (eventKind, 
                                                                                     m_owner.shared_from_this());
        data->GetBreakpointLocationCollection().Add (shared_from_this());
        m_owner.GetTarget().BroadcastEvent (Target::eBroadcastBitBreakpointChanged, data);
    }
}
    
