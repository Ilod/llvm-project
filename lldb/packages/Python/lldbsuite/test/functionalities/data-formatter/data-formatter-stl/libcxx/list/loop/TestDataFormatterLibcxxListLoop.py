"""
Test that the debugger handles loops in std::list (which can appear as a result of e.g. memory
corruption).
"""

from __future__ import print_function



import os, time, re
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.lldbutil as lldbutil

class LibcxxListDataFormatterTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    @skipIfGcc
    @skipIfWindows # libc++ not ported to Windows yet
    @add_test_categories(["pyapi"])
    def test_with_run_command(self):
        self.build()
        exe = os.path.join(os.getcwd(), "a.out")
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target and target.IsValid(), "Target is valid")

        file_spec = lldb.SBFileSpec ("main.cpp", False)
        breakpoint1 = target.BreakpointCreateBySourceRegex('// Set break point at this line.', file_spec)
        self.assertTrue(breakpoint1 and breakpoint1.IsValid())
        breakpoint2 = target.BreakpointCreateBySourceRegex('// Set second break point at this line.', file_spec)
        self.assertTrue(breakpoint2 and breakpoint2.IsValid())

        # Run the program, it should stop at breakpoint 1.
        process = target.LaunchSimple(None, None, self.get_process_working_directory())
        lldbutil.skip_if_library_missing(self, target, lldbutil.PrintableRegex("libc\+\+"))
        self.assertTrue(process and process.IsValid(), PROCESS_IS_VALID)
        self.assertEqual(len(lldbutil.get_threads_stopped_at_breakpoint(process, breakpoint1)), 1)

        # verify our list is displayed correctly
        self.expect("frame variable *numbers_list", substrs=['[0] = 1', '[1] = 2', '[2] = 3', '[3] = 4', '[5] = 6'])

        # Continue to breakpoint 2.
        process.Continue()
        self.assertTrue(process and process.IsValid(), PROCESS_IS_VALID)
        self.assertEqual(len(lldbutil.get_threads_stopped_at_breakpoint(process, breakpoint2)), 1)

        # The list is now inconsistent. However, we should be able to get the first three
        # elements at least (and most importantly, not crash).
        self.expect("frame variable *numbers_list", substrs=['[0] = 1', '[1] = 2', '[2] = 3'])

        # Run to completion.
        process.Continue()
        self.assertEqual(process.GetState(), lldb.eStateExited, PROCESS_EXITED)
