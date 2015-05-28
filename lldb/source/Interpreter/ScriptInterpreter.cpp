//===-- ScriptInterpreter.cpp -----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#include "lldb/Interpreter/ScriptInterpreter.h"

#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "lldb/Core/Error.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/StringList.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#include "lldb/Utility/PseudoTerminal.h"

using namespace lldb;
using namespace lldb_private;

ScriptInterpreter::ScriptInterpreter (CommandInterpreter &interpreter, lldb::ScriptLanguage script_lang) :
    m_interpreter (interpreter),
    m_script_lang (script_lang)
{
}

ScriptInterpreter::~ScriptInterpreter ()
{
}

CommandInterpreter &
ScriptInterpreter::GetCommandInterpreter ()
{
    return m_interpreter;
}

void 
ScriptInterpreter::CollectDataForBreakpointCommandCallback 
(
    std::vector<BreakpointOptions *> &bp_options_vec,
    CommandReturnObject &result
)
{
    result.SetStatus (eReturnStatusFailed);
    result.AppendError ("ScriptInterpreter::GetScriptCommands(StringList &) is not implemented.");
}

void 
ScriptInterpreter::CollectDataForWatchpointCommandCallback 
(
    WatchpointOptions *bp_options,
    CommandReturnObject &result
)
{
    result.SetStatus (eReturnStatusFailed);
    result.AppendError ("ScriptInterpreter::GetScriptCommands(StringList &) is not implemented.");
}

std::string
ScriptInterpreter::LanguageToString (lldb::ScriptLanguage language)
{
    std::string return_value;

    switch (language)
    {
        case eScriptLanguageNone:
            return_value = "None";
            break;
        case eScriptLanguagePython:
            return_value = "Python";
            break;
    }

    return return_value;
}

Error
ScriptInterpreter::SetBreakpointCommandCallback (std::vector<BreakpointOptions *> &bp_options_vec,
                                                 const char *callback_text)
{
    Error return_error;
    for (BreakpointOptions *bp_options : bp_options_vec)
    {
        return_error = SetBreakpointCommandCallback(bp_options, callback_text);
        if (return_error.Success())
            break;
    }
    return return_error;
}

void
ScriptInterpreter::SetBreakpointCommandCallbackFunction (std::vector<BreakpointOptions *> &bp_options_vec,
                                                         const char *function_name)
{
    for (BreakpointOptions *bp_options : bp_options_vec)
    {
        SetBreakpointCommandCallbackFunction(bp_options, function_name);
    }
}

std::unique_ptr<ScriptInterpreterLocker>
ScriptInterpreter::AcquireInterpreterLock ()
{
    return std::unique_ptr<ScriptInterpreterLocker>(new ScriptInterpreterLocker());
}
