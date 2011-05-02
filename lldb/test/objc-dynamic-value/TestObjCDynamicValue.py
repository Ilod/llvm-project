"""
Use lldb Python API to test dynamic values in ObjC
"""

import os, time
import re
import unittest2
import lldb, lldbutil
from lldbtest import *

class ObjCDynamicValueTestCase(TestBase):

    mydir = "objc-dynamic-value"

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @python_api_test
    def test_get_dynamic_objc_vals_with_dsym(self):
        """Test fetching ObjC dynamic values."""
        self.buildDsym()
        self.do_get_dynamic_vals()

    @python_api_test
    def test_get_objc_dynamic_vals_with_dwarf(self):
        """Test fetching ObjC dynamic values."""
        self.buildDwarf()
        self.do_get_dynamic_vals()

    def setUp(self):
        # Call super's setUp().                                                                                                           
        TestBase.setUp(self)

        # Find the line number to break for main.c.                                                                                       

        self.source_name = 'dynamic-value.m'
        self.set_property_line = line_number(self.source_name, '// This is the line in setProperty, make sure we step to here.')
        self.handle_SourceBase = line_number(self.source_name,
                                                 '// Break here to check dynamic values.')
        self.main_before_setProperty_line = line_number(self.source_name,
                                                       '// Break here to see if we can step into real method.')

    def examine_SourceDerived_ptr (self, object):
        self.assertTrue (object.IsValid())
        self.assertTrue (object.GetTypeName().find ('SourceDerived') != -1)
        derivedValue = object.GetChildMemberWithName ('_derivedValue')
        self.assertTrue (derivedValue.IsValid())
        self.assertTrue (int (derivedValue.GetValue(), 0) == 30)

    def do_get_dynamic_vals(self):
        """Make sure we get dynamic values correctly both for compiled in classes and dynamic ones"""
        exe = os.path.join(os.getcwd(), "a.out")

        # Create a target from the debugger.

        target = self.dbg.CreateTarget (exe)
        self.assertTrue(target.IsValid(), VALID_TARGET)

        # Set up our breakpoints:

        handle_SourceBase_bkpt = target.BreakpointCreateByLocation(self.source_name, self.handle_SourceBase)
        self.assertTrue(handle_SourceBase_bkpt.IsValid() and
                        handle_SourceBase_bkpt.GetNumLocations() == 1,
                        VALID_BREAKPOINT)

        main_before_setProperty_bkpt = target.BreakpointCreateByLocation(self.source_name, self.main_before_setProperty_line)
        self.assertTrue(main_before_setProperty_bkpt.IsValid() and
                        main_before_setProperty_bkpt.GetNumLocations() == 1,
                        VALID_BREAKPOINT)

        # Now launch the process, and do not stop at the entry point.
        self.process = target.LaunchSimple (None, None, os.getcwd())

        self.assertTrue(self.process.GetState() == lldb.eStateStopped,
                        PROCESS_STOPPED)

        threads = lldbutil.get_threads_stopped_at_breakpoint (self.process, main_before_setProperty_bkpt)
        self.assertTrue (len(threads) == 1)
        thread = threads[0]

        #
        #  At this point, myObserver has a Source pointer that is actually a KVO swizzled SourceDerived
        #  make sure we can get that properly:

        frame = thread.GetFrameAtIndex(0)
        myObserver = frame.FindVariable('myObserver')
        self.assertTrue (myObserver.IsValid())
        myObserver_source = myObserver.GetChildMemberWithName ('_source')
        self.examine_SourceDerived_ptr (myObserver_source)

        # The "frame var" code uses another path to get into children, so let's
        # make sure that works as well:

        result = lldb.SBCommandReturnObject()

        self.expect('frame var -d 1 myObserver->_source', 'frame var finds its way into a child member',
            patterns = ['\(SourceDerived \*\)'])

        # This test is not entirely related to the main thrust of this test case, but since we're here,
        # try stepping into setProperty, and make sure we get into the version in Source:

        thread.StepInto()

        threads = lldbutil.get_stopped_threads (self.process, lldb.eStopReasonPlanComplete)
        self.assertTrue (len(threads) == 1)
        line_entry = threads[0].GetFrameAtIndex(0).GetLineEntry()
        self.assertTrue (line_entry.GetLine() == self.set_property_line)
        self.assertTrue (line_entry.GetFileSpec().GetFilename() == self.source_name) 

        # Okay, back to the main business.  Continue to the handle_SourceBase and make sure we get the correct dynamic value.

        threads = lldbutil.continue_to_breakpoint (self.process, handle_SourceBase_bkpt)
        self.assertTrue (len(threads) == 1)
        thread = threads[0]

        frame = thread.GetFrameAtIndex(0)

        # Get "object" using FindVariable:

        noDynamic = False
        useDynamic = True

        object_static = frame.FindVariable ('object', noDynamic)
        object_dynamic = frame.FindVariable ('object', useDynamic)

        # Delete this object to make sure that this doesn't cause havoc with the dynamic object that depends on it.
        del (object_static)

        self.examine_SourceDerived_ptr (object_dynamic)
        
        # Get "this" using FindValue, make sure that works too:
        object_static = frame.FindValue ('object', lldb.eValueTypeVariableArgument, noDynamic)
        object_dynamic = frame.FindValue ('object', lldb.eValueTypeVariableArgument, useDynamic)
        del (object_static)
        self.examine_SourceDerived_ptr (object_dynamic)

        # Get "this" using the EvaluateExpression:
        # These tests fail for now because EvaluateExpression doesn't currently support dynamic typing...
        #object_static = frame.EvaluateExpression ('object', False)
        #object_dynamic = frame.EvaluateExpression ('object', True)
        #self.examine_value_object_of_object_ptr (object_static, object_dynamic, myB_loc)
        
        # Continue again to the handle_SourceBase and make sure we get the correct dynamic value.
        # This one looks exactly the same, but in fact this is an "un-KVO'ed" version of SourceBase, so
        # its isa pointer points to SourceBase not NSKVOSourceBase or whatever...

        threads = lldbutil.continue_to_breakpoint (self.process, handle_SourceBase_bkpt)
        self.assertTrue (len(threads) == 1)
        thread = threads[0]

        frame = thread.GetFrameAtIndex(0)

        # Get "object" using FindVariable:

        object_static = frame.FindVariable ('object', noDynamic)
        object_dynamic = frame.FindVariable ('object', useDynamic)

        # Delete this object to make sure that this doesn't cause havoc with the dynamic object that depends on it.
        del (object_static)

        self.examine_SourceDerived_ptr (object_dynamic)

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
