"""
Test conditionally break on a function and inspect its variables.
"""

import os, time
import re
import unittest2
import lldb, lldbutil
from lldbtest import *

class ConditionalBreakTestCase(TestBase):

    mydir = "conditional_break"

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    def test_with_dsym_python(self):
        """Exercise some thread and frame APIs to break if c() is called by a()."""
        self.buildDsym()
        self.do_conditional_break()

    def test_with_dwarf_python(self):
        """Exercise some thread and frame APIs to break if c() is called by a()."""
        self.buildDwarf()
        self.do_conditional_break()

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    def test_with_dsym_command(self):
        """Simulate a user using lldb commands to break on c() if called from a()."""
        self.buildDsym()
        self.simulate_conditional_break_by_user()

    def test_with_dwarf_command(self):
        """Simulate a user using lldb commands to break on c() if called from a()."""
        self.buildDwarf()
        self.simulate_conditional_break_by_user()

    def do_conditional_break(self):
        """Exercise some thread and frame APIs to break if c() is called by a()."""
        exe = os.path.join(os.getcwd(), "a.out")

        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target.IsValid(), VALID_TARGET)

        breakpoint = target.BreakpointCreateByName("c", exe)
        self.assertTrue(breakpoint.IsValid(), VALID_BREAKPOINT)

        # Now launch the process, and do not stop at entry point.
        rc = lldb.SBError()
        self.process = target.Launch([''], [''], os.ctermid(), 0, False, rc)

        self.assertTrue(rc.Success() and self.process.IsValid(), PROCESS_IS_VALID)

        # The stop reason of the thread should be breakpoint.
        self.assertTrue(self.process.GetState() == lldb.eStateStopped,
                        STOPPED_DUE_TO_BREAKPOINT)

        # Suppose we are only interested in the call scenario where c()'s
        # immediate caller is a() and we want to find out the value passed from
        # a().
        #
        # The 10 in range(10) is just an arbitrary number, which means we would
        # like to try for at most 10 times.
        for j in range(10):
            thread = self.process.GetThreadAtIndex(0)
            
            if thread.GetNumFrames() >= 2:
                frame0 = thread.GetFrameAtIndex(0)
                name0 = frame0.GetFunction().GetName()
                frame1 = thread.GetFrameAtIndex(1)
                name1 = frame1.GetFunction().GetName()
                #lldbutil.PrintStackTrace(thread)
                self.assertTrue(name0 == "c", "Break on function c()")
                if (name1 == "a"):
                    line = frame1.GetLineEntry().GetLine()
                    # By design, we know that a() calls c() only from main.c:27.
                    # In reality, similar logic can be used to find out the call
                    # site.
                    self.assertTrue(line == 27, "Immediate caller a() at main.c:27")

                    # And the local variable 'val' should have a value of (int) 3.
                    val = frame1.LookupVar("val")
                    self.assertTrue(val.GetTypeName() == "int", "'val' has int type")
                    self.assertTrue(val.GetValue(frame1) == "3", "'val' has a value of 3")
                    break

            self.process.Continue()

    def simulate_conditional_break_by_user(self):
        """Simulate a user using lldb commands to break on c() if called from a()."""

        # Sourcing .lldb in the current working directory, which sets the main
        # executable, sets the breakpoint on c(), and adds the callback for the
        # breakpoint such that lldb only stops when the caller of c() is a().
        # the "my" package that defines the date() function.
        self.runCmd("command source .lldb")

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['state is Stopped', 'stop reason = breakpoint'])

        # The frame info for frame #0 points to a.out`c and its immediate caller
        # (frame #1) points to a.out`a.

        self.expect("frame info", "We should stop at c()",
            substrs = ["a.out`c"])

        # Select our parent frame as the current frame.
        self.runCmd("frame select 1")
        self.expect("frame info", "The immediate caller should be a()",
            substrs = ["a.out`a"])



if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
