"""
Test the 'register' command.
"""

from __future__ import print_function



import os, sys, time
import re
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.lldbutil as lldbutil

class RegisterCommandsTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    def setUp(self):
        TestBase.setUp(self)
        self.has_teardown = False

    def tearDown(self):
        self.dbg.GetSelectedTarget().GetProcess().Destroy()
        TestBase.tearDown(self)

    def test_register_commands(self):
        """Test commands related to registers, in particular vector registers."""
        if not self.getArchitecture() in ['amd64', 'i386', 'x86_64']:
            self.skipTest("This test requires x86 or x86_64 as the architecture for the inferior")
        self.build()
        self.register_commands()

    @skipIfTargetAndroid(archs=["i386"]) # Writing of mxcsr register fails, presumably due to a kernel/hardware problem
    def test_fp_register_write(self):
        """Test commands that write to registers, in particular floating-point registers."""
        if not self.getArchitecture() in ['amd64', 'i386', 'x86_64']:
            self.skipTest("This test requires x86 or x86_64 as the architecture for the inferior")
        self.build()
        self.fp_register_write()

    @expectedFailureAndroid(archs=["i386"]) # "register read fstat" always return 0xffff
    @skipIfFreeBSD    #llvm.org/pr25057
    def test_fp_special_purpose_register_read(self):
        """Test commands that read fpu special purpose registers."""
        if not self.getArchitecture() in ['amd64', 'i386', 'x86_64']:
            self.skipTest("This test requires x86 or x86_64 as the architecture for the inferior")
        self.build()
        self.fp_special_purpose_register_read()

    def test_register_expressions(self):
        """Test expression evaluation with commands related to registers."""
        if not self.getArchitecture() in ['amd64', 'i386', 'x86_64']:
            self.skipTest("This test requires x86 or x86_64 as the architecture for the inferior")
        self.build()
        self.register_expressions()

    def test_convenience_registers(self):
        """Test convenience registers."""
        if not self.getArchitecture() in ['amd64', 'x86_64']:
            self.skipTest("This test requires x86_64 as the architecture for the inferior")
        self.build()
        self.convenience_registers()

    def test_convenience_registers_with_process_attach(self):
        """Test convenience registers after a 'process attach'."""
        if not self.getArchitecture() in ['amd64', 'x86_64']:
            self.skipTest("This test requires x86_64 as the architecture for the inferior")
        self.build()
        self.convenience_registers_with_process_attach(test_16bit_regs=False)

    def test_convenience_registers_16bit_with_process_attach(self):
        """Test convenience registers after a 'process attach'."""
        if not self.getArchitecture() in ['amd64', 'x86_64']:
            self.skipTest("This test requires x86_64 as the architecture for the inferior")
        self.build()
        self.convenience_registers_with_process_attach(test_16bit_regs=True)

    def common_setup(self):
        exe = os.path.join(os.getcwd(), "a.out")

        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        # Break in main().
        lldbutil.run_break_set_by_symbol (self, "main", num_expected_locations=-1)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['stopped', 'stop reason = breakpoint'])

    def remove_log(self):
        """ Remove the temporary log file generated by some tests."""
        if os.path.exists(self.log_file):
            os.remove(self.log_file)

    # platform specific logging of the specified category
    def log_enable(self, category):
        # This intentionally checks the host platform rather than the target
        # platform as logging is host side.
        self.platform = ""
        if sys.platform.startswith("darwin"):
            self.platform = "" # TODO: add support for "log enable darwin registers"

        if sys.platform.startswith("freebsd"):
            self.platform = "freebsd"

        if sys.platform.startswith("linux"):
            self.platform = "linux"

        if self.platform != "":
            self.log_file = os.path.join(os.getcwd(), 'TestRegisters.log')
            self.runCmd("log enable " + self.platform + " " + str(category) + " registers -v -f " + self.log_file, RUN_SUCCEEDED)
            if not self.has_teardown:
                self.has_teardown = True
                self.addTearDownHook(self.remove_log)

    def register_commands(self):
        """Test commands related to registers, in particular vector registers."""
        self.common_setup()

        # verify that logging does not assert
        self.log_enable("registers")

        self.expect("register read -a", MISSING_EXPECTED_REGISTERS,
            substrs = ['registers were unavailable'], matching = False)
        self.runCmd("register read xmm0")
        self.runCmd("register read ymm15") # may be available

        self.expect("register read -s 3",
            substrs = ['invalid register set index: 3'], error = True)

    def write_and_restore(self, frame, register, must_exist = True):
        value = frame.FindValue(register, lldb.eValueTypeRegister)
        if must_exist:
            self.assertTrue(value.IsValid(), "finding a value for register " + register)
        elif not value.IsValid():
            return # If register doesn't exist, skip this test

        error = lldb.SBError()
        register_value = value.GetValueAsUnsigned(error, 0)
        self.assertTrue(error.Success(), "reading a value for " + register)

        self.runCmd("register write " + register + " 0xff0e")
        self.expect("register read " + register,
            substrs = [register + ' = 0x', 'ff0e'])

        self.runCmd("register write " + register + " " + str(register_value))
        self.expect("register read " + register,
            substrs = [register + ' = 0x'])

    def vector_write_and_read(self, frame, register, new_value, must_exist = True):
        value = frame.FindValue(register, lldb.eValueTypeRegister)
        if must_exist:
            self.assertTrue(value.IsValid(), "finding a value for register " + register)
        elif not value.IsValid():
            return # If register doesn't exist, skip this test

        self.runCmd("register write " + register + " \'" + new_value + "\'")
        self.expect("register read " + register,
            substrs = [register + ' = ', new_value])

    def fp_special_purpose_register_read(self):
        exe = os.path.join(os.getcwd(), "a.out")

        # Create a target by the debugger.
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        # Launch the process and stop.
        self.expect ("run", PROCESS_STOPPED, substrs = ['stopped'])

        # Check stop reason; Should be either signal SIGTRAP or EXC_BREAKPOINT
        output = self.res.GetOutput()
        matched = False
        substrs = ['stop reason = EXC_BREAKPOINT', 'stop reason = signal SIGTRAP']
        for str1 in substrs:
            matched = output.find(str1) != -1
            with recording(self, False) as sbuf:
                print("%s sub string: %s" % ('Expecting', str1), file=sbuf)
                print("Matched" if matched else "Not Matched", file=sbuf)
            if matched:
                break
        self.assertTrue(matched, STOPPED_DUE_TO_SIGNAL)

        process = target.GetProcess()
        self.assertTrue(process.GetState() == lldb.eStateStopped,
                        PROCESS_STOPPED)

        thread = process.GetThreadAtIndex(0)
        self.assertTrue(thread.IsValid(), "current thread is valid")

        currentFrame = thread.GetFrameAtIndex(0)
        self.assertTrue(currentFrame.IsValid(), "current frame is valid")

        # Extract the value of fstat and ftag flag at the point just before
        # we start pushing floating point values on st% register stack
        value = currentFrame.FindValue("fstat", lldb.eValueTypeRegister)
        error = lldb.SBError()
        reg_value_fstat_initial = value.GetValueAsUnsigned(error, 0)

        self.assertTrue(error.Success(), "reading a value for fstat")
        value = currentFrame.FindValue("ftag", lldb.eValueTypeRegister)
        error = lldb.SBError()
        reg_value_ftag_initial = value.GetValueAsUnsigned(error, 0)

        self.assertTrue(error.Success(), "reading a value for ftag")
        fstat_top_pointer_initial = (reg_value_fstat_initial & 0x3800)>>11

        # Execute 'si' aka 'thread step-inst' instruction 5 times and with
        # every execution verify the value of fstat and ftag registers
        for x in range(0,5):
            # step into the next instruction to push a value on 'st' register stack
            self.runCmd ("si", RUN_SUCCEEDED)

            # Verify fstat and save it to be used for verification in next execution of 'si' command
            if not (reg_value_fstat_initial & 0x3800):
                self.expect("register read fstat",
                    substrs = ['fstat' + ' = ', str("0x%0.4x" %((reg_value_fstat_initial & ~(0x3800))| 0x3800))])
                reg_value_fstat_initial = ((reg_value_fstat_initial & ~(0x3800))| 0x3800)
                fstat_top_pointer_initial = 7
            else :
                self.expect("register read fstat",
                    substrs = ['fstat' + ' = ', str("0x%0.4x" % (reg_value_fstat_initial - 0x0800))])
                reg_value_fstat_initial = (reg_value_fstat_initial - 0x0800)
                fstat_top_pointer_initial -= 1

            # Verify ftag and save it to be used for verification in next execution of 'si' command
            self.expect("register read ftag",
                substrs = ['ftag' + ' = ', str("0x%0.2x" % (reg_value_ftag_initial | (1<< fstat_top_pointer_initial)))])
            reg_value_ftag_initial = reg_value_ftag_initial | (1<< fstat_top_pointer_initial)

    def fp_register_write(self):
        exe = os.path.join(os.getcwd(), "a.out")

        # Create a target by the debugger.
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        lldbutil.run_break_set_by_symbol (self, "main", num_expected_locations=-1)

        # Launch the process, and do not stop at the entry point.
        process = target.LaunchSimple (None, None, self.get_process_working_directory())

        process = target.GetProcess()
        self.assertTrue(process.GetState() == lldb.eStateStopped,
                        PROCESS_STOPPED)

        thread = process.GetThreadAtIndex(0)
        self.assertTrue(thread.IsValid(), "current thread is valid")

        currentFrame = thread.GetFrameAtIndex(0)
        self.assertTrue(currentFrame.IsValid(), "current frame is valid")

        self.write_and_restore(currentFrame, "fcw", False)
        self.write_and_restore(currentFrame, "fsw", False)
        self.write_and_restore(currentFrame, "ftw", False)
        self.write_and_restore(currentFrame, "ip", False)
        self.write_and_restore(currentFrame, "dp", False)
        self.write_and_restore(currentFrame, "mxcsr", False)
        self.write_and_restore(currentFrame, "mxcsrmask", False)

        st0regname = "st0"
        if currentFrame.FindRegister(st0regname).IsValid() == False:
                st0regname = "stmm0"
        if currentFrame.FindRegister(st0regname).IsValid() == False:
                return # TODO: anything smarter here

        new_value = "{0x01 0x02 0x03 0x00 0x00 0x00 0x00 0x00 0x00 0x00}"
        self.vector_write_and_read(currentFrame, st0regname, new_value)

        new_value = "{0x01 0x02 0x03 0x00 0x00 0x00 0x00 0x00 0x09 0x0a 0x2f 0x2f 0x2f 0x2f 0x2f 0x2f}"
        self.vector_write_and_read(currentFrame, "xmm0", new_value)
        new_value = "{0x01 0x02 0x03 0x00 0x00 0x00 0x00 0x00 0x09 0x0a 0x2f 0x2f 0x2f 0x2f 0x0e 0x0f}"
        self.vector_write_and_read(currentFrame, "xmm15", new_value, False)

        self.runCmd("register write " + st0regname + " \"{0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00}\"")
        self.expect("register read " + st0regname + " --format f",
            substrs = [st0regname + ' = 0'])

        has_avx = False 
        registerSets = currentFrame.GetRegisters() # Returns an SBValueList.
        for registerSet in registerSets:
            if 'advanced vector extensions' in registerSet.GetName().lower():
                has_avx = True
                break

        if has_avx:
            new_value = "{0x01 0x02 0x03 0x00 0x00 0x00 0x00 0x00 0x09 0x0a 0x2f 0x2f 0x2f 0x2f 0x0e 0x0f 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x0c 0x0d 0x0e 0x0f}"
            self.vector_write_and_read(currentFrame, "ymm0", new_value)
            self.vector_write_and_read(currentFrame, "ymm7", new_value)
            self.expect("expr $ymm0", substrs = ['vector_type'])
        else:
            self.runCmd("register read ymm0")

    def register_expressions(self):
        """Test expression evaluation with commands related to registers."""
        self.common_setup()

        self.expect("expr/x $eax",
            substrs = ['unsigned int', ' = 0x'])

        if self.getArchitecture() in ['amd64', 'x86_64']:
            self.expect("expr -- ($rax & 0xffffffff) == $eax",
                substrs = ['true'])

        self.expect("expr $xmm0",
            substrs = ['vector_type'])

        self.expect("expr (unsigned int)$xmm0[0]",
            substrs = ['unsigned int'])

    def convenience_registers(self):
        """Test convenience registers."""
        self.common_setup()

        # The command "register read -a" does output a derived register like eax...
        self.expect("register read -a", matching=True,
            substrs = ['eax'])

        # ...however, the vanilla "register read" command should not output derived registers like eax.
        self.expect("register read", matching=False,
            substrs = ['eax'])
        
        # Test reading of rax and eax.
        self.expect("register read rax eax",
            substrs = ['rax = 0x', 'eax = 0x'])

        # Now write rax with a unique bit pattern and test that eax indeed represents the lower half of rax.
        self.runCmd("register write rax 0x1234567887654321")
        self.expect("register read rax 0x1234567887654321",
            substrs = ['0x1234567887654321'])

    def convenience_registers_with_process_attach(self, test_16bit_regs):
        """Test convenience registers after a 'process attach'."""
        exe = os.path.join(os.getcwd(), "a.out")

        # Spawn a new process
        pid = self.spawnSubprocess(exe, ['wait_for_attach']).pid
        self.addTearDownHook(self.cleanupSubprocesses)

        if self.TraceOn():
            print("pid of spawned process: %d" % pid)

        self.runCmd("process attach -p %d" % pid)

        # Check that "register read eax" works.
        self.runCmd("register read eax")

        if self.getArchitecture() in ['amd64', 'x86_64']:
            self.expect("expr -- ($rax & 0xffffffff) == $eax",
                substrs = ['true'])

        if test_16bit_regs:
            self.expect("expr -- $ax == (($ah << 8) | $al)",
                substrs = ['true'])
