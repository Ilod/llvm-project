import sys
import lldb
import lldbutil

def stop_if_called_from_a():
    # lldb.debugger_unique_id stores the id of the debugger associated with us.
    dbg = lldb.SBDebugger.FindDebuggerWithID(lldb.debugger_unique_id)

    # Perform synchronous interaction with the debugger.
    dbg.SetAsync(False)

    # Get the command interpreter.
    ci = dbg.GetCommandInterpreter()

    # And the result object for ci interaction.
    res = lldb.SBCommandReturnObject()

    # Retrieve the target, process, and the only thread.
    target = dbg.GetSelectedTarget()
    process = target.GetProcess()
    thread = process.GetThreadAtIndex(0)

    # We check the call frames in order to stop only when the immediate caller
    # of the leaf function c() is a().  If it's not the right caller, we ask the
    # command interpreter to continue execution.

    print >> sys.stderr, "Checking call frames..."
    lldbutil.PrintStackTrace(thread)
    if thread.GetNumFrames() >= 2:
        funcs = lldbutil.GetFunctionNames(thread)
        print >> sys.stderr, funcs[0], "called from", funcs[1]
        if (funcs[0] == 'c' and funcs[1] == 'a'):
            print >> sys.stderr, "Stopped at c() with immediate caller as a()."
        else:
            print >> sys.stderr, "Continuing..."
            ci.HandleCommand("process continue", res)

    return True

