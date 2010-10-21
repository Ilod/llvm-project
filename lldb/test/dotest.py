#!/usr/bin/env python

"""
A simple testing framework for lldb using python's unit testing framework.

Tests for lldb are written as python scripts which take advantage of the script
bridging provided by LLDB.framework to interact with lldb core.

A specific naming pattern is followed by the .py script to be recognized as
a module which implements a test scenario, namely, Test*.py.

To specify the directories where "Test*.py" python test scripts are located,
you need to pass in a list of directory names.  By default, the current
working directory is searched if nothing is specified on the command line.

Type:

./dotest.py -h

for available options.
"""

import os, signal, sys, time
import unittest2

class _WritelnDecorator(object):
    """Used to decorate file-like objects with a handy 'writeln' method"""
    def __init__(self,stream):
        self.stream = stream

    def __getattr__(self, attr):
        if attr in ('stream', '__getstate__'):
            raise AttributeError(attr)
        return getattr(self.stream,attr)

    def writeln(self, arg=None):
        if arg:
            self.write(arg)
        self.write('\n') # text-mode streams translate to \r\n if needed

#
# Global variables:
#

# The test suite.
suite = unittest2.TestSuite()

# The config file is optional.
configFile = None

# The dictionary as a result of sourcing configFile.
config = {}

# Delay startup in order for the debugger to attach.
delay = False

# The filter (testcase.testmethod) used to admit tests into our test suite.
filterspec = None

# If '-g' is specified, the filterspec must be consulted for each test module, default to False.
fs4all = False

# Ignore the build search path relative to this script to locate the lldb.py module.
ignore = False

# By default, we skip long running test case.  Use '-l' option to override.
skipLongRunningTest = True

# The regular expression pattern to match against eligible filenames as our test cases.
regexp = None

# By default, tests are executed in place and cleanups are performed afterwards.
# Use '-r dir' option to relocate the tests and their intermediate files to a
# different directory and to forgo any cleanups.  The directory specified must
# not exist yet.
rdir = None

# Default verbosity is 0.
verbose = 0

# By default, search from the current working directory.
testdirs = [ os.getcwd() ]

# Separator string.
separator = '-' * 70


def usage():
    print """
Usage: dotest.py [option] [args]
where options:
-h   : print this help message and exit (also --help)
-c   : read a config file specified after this option
       (see also lldb-trunk/example/test/usage-config)
-d   : delay startup for 10 seconds (in order for the debugger to attach)
-f   : specify a filter, which consists of the test class name, a dot, followed by
       the test method, to admit tests into the test suite
       e.g., -f 'ClassTypesTestCase.test_with_dwarf_and_python_api'
-g   : if specified, only the modules with the corect filter will be run
       it has no effect if no '-f' option is present
       '-f filterspec -g' can be used with '-p filename-regexp' to select only
       the testfile.testclass.testmethod to run
-i   : ignore (don't bailout) if 'lldb.py' module cannot be located in the build
       tree relative to this script; use PYTHONPATH to locate the module
-l   : don't skip long running test
-p   : specify a regexp filename pattern for inclusion in the test suite
-r   : specify a dir to relocate the tests and their intermediate files to;
       the directory must not exist before running this test driver;
       no cleanup of intermediate test files is performed in this case
-t   : trace lldb command execution and result
-v   : do verbose mode of unittest framework
-w   : insert some wait time (currently 0.5 sec) between consecutive test cases

and:
args : specify a list of directory names to search for python Test*.py scripts
       if empty, search from the curret working directory, instead

This is an example of using the -f -g options to pinpoint to a specfic test
method to be run:

$ ./dotest.py -f ClassTypesTestCase.test_with_dsym_and_run_command -g
----------------------------------------------------------------------
Collected 1 test

test_with_dsym_and_run_command (TestClassTypes.ClassTypesTestCase)
Test 'frame variable this' when stopped on a class constructor. ... ok

----------------------------------------------------------------------
Ran 1 test in 1.396s

OK
$ 

Running of this script also sets up the LLDB_TEST environment variable so that
individual test cases can locate their supporting files correctly.  The script
tries to set up Python's search paths for modules by looking at the build tree
relative to this script.  See also the '-i' option.

Environment variables related to loggings:

o LLDB_LOG: if defined, specifies the log file pathname for the 'lldb' subsystem
  with a default option of 'event process' if LLDB_LOG_OPTION is not defined.

o GDB_REMOTE_LOG: if defined, specifies the log file pathname for the
  'process.gdb-remote' subsystem with a default option of 'packets' if
  GDB_REMOTE_LOG_OPTION is not defined.
"""
    sys.exit(0)


def parseOptionsAndInitTestdirs():
    """Initialize the list of directories containing our unittest scripts.

    '-h/--help as the first option prints out usage info and exit the program.
    """

    global configFile
    global delay
    global filterspec
    global fs4all
    global ignore
    global skipLongRunningTest
    global regexp
    global rdir
    global verbose
    global testdirs

    if len(sys.argv) == 1:
        return

    # Process possible trace and/or verbose flag, among other things.
    index = 1
    while index < len(sys.argv):
        if not sys.argv[index].startswith('-'):
            # End of option processing.
            break

        if sys.argv[index].find('-h') != -1:
            usage()
        elif sys.argv[index].startswith('-c'):
            # Increment by 1 to fetch the config file name option argument.
            index += 1
            if index >= len(sys.argv) or sys.argv[index].startswith('-'):
                usage()
            configFile = sys.argv[index]
            if not os.path.isfile(configFile):
                print "Config file:", configFile, "does not exist!"
                usage()
            index += 1
        elif sys.argv[index].startswith('-d'):
            delay = True
            index += 1
        elif sys.argv[index].startswith('-f'):
            # Increment by 1 to fetch the filter spec.
            index += 1
            if index >= len(sys.argv) or sys.argv[index].startswith('-'):
                usage()
            filterspec = sys.argv[index]
            index += 1
        elif sys.argv[index].startswith('-g'):
            fs4all = True
            index += 1
        elif sys.argv[index].startswith('-i'):
            ignore = True
            index += 1
        elif sys.argv[index].startswith('-l'):
            skipLongRunningTest = False
            index += 1
        elif sys.argv[index].startswith('-p'):
            # Increment by 1 to fetch the reg exp pattern argument.
            index += 1
            if index >= len(sys.argv) or sys.argv[index].startswith('-'):
                usage()
            regexp = sys.argv[index]
            index += 1
        elif sys.argv[index].startswith('-r'):
            # Increment by 1 to fetch the relocated directory argument.
            index += 1
            if index >= len(sys.argv) or sys.argv[index].startswith('-'):
                usage()
            rdir = os.path.abspath(sys.argv[index])
            if os.path.exists(rdir):
                print "Relocated directory:", rdir, "must not exist!"
                usage()
            index += 1
        elif sys.argv[index].startswith('-t'):
            os.environ["LLDB_COMMAND_TRACE"] = "YES"
            index += 1
        elif sys.argv[index].startswith('-v'):
            verbose = 2
            index += 1
        elif sys.argv[index].startswith('-w'):
            os.environ["LLDB_WAIT_BETWEEN_TEST_CASES"] = 'YES'
            index += 1
        else:
            print "Unknown option: ", sys.argv[index]
            usage()

    # Gather all the dirs passed on the command line.
    if len(sys.argv) > index:
        testdirs = map(os.path.abspath, sys.argv[index:])

    # If '-r dir' is specified, the tests should be run under the relocated
    # directory.  Let's copy the testdirs over.
    if rdir:
        from shutil import copytree, ignore_patterns

        tmpdirs = []
        for srcdir in testdirs:
            dstdir = os.path.join(rdir, os.path.basename(srcdir))
            # Don't copy the *.pyc and .svn stuffs.
            copytree(srcdir, dstdir, ignore=ignore_patterns('*.pyc', '.svn'))
            tmpdirs.append(dstdir)

        # This will be our modified testdirs.
        testdirs = tmpdirs

        # With '-r dir' specified, there's no cleanup of intermediate test files.
        os.environ["LLDB_DO_CLEANUP"] = 'NO'

        # If testdirs is ['test'], the make directory has already been copied
        # recursively and is contained within the rdir/test dir.  For anything
        # else, we would need to copy over the make directory and its contents,
        # so that, os.listdir(rdir) looks like, for example:
        #
        #     array_types conditional_break make
        #
        # where the make directory contains the Makefile.rules file.
        if len(testdirs) != 1 or os.path.basename(testdirs[0]) != 'test':
            # Don't copy the .svn stuffs.
            copytree('make', os.path.join(rdir, 'make'),
                     ignore=ignore_patterns('.svn'))

    #print "testdirs:", testdirs

    # Source the configFile if specified.
    # The side effect, if any, will be felt from this point on.  An example
    # config file may be these simple two lines:
    #
    # sys.stderr = open("/tmp/lldbtest-stderr", "w")
    # sys.stdout = open("/tmp/lldbtest-stdout", "w")
    #
    # which will reassign the two file objects to sys.stderr and sys.stdout,
    # respectively.
    #
    # See also lldb-trunk/example/test/usage-config.
    global config
    if configFile:
        # Pass config (a dictionary) as the locals namespace for side-effect.
        execfile(configFile, globals(), config)
        #print "config:", config
        #print "sys.stderr:", sys.stderr
        #print "sys.stdout:", sys.stdout


def setupSysPath():
    """Add LLDB.framework/Resources/Python to the search paths for modules."""

    global rdir
    global testdirs

    # Get the directory containing the current script.
    scriptPath = sys.path[0]
    if not scriptPath.endswith('test'):
        print "This script expects to reside in lldb's test directory."
        sys.exit(-1)

    if rdir:
        # Set up the LLDB_TEST environment variable appropriately, so that the
        # individual tests can be located relatively.
        #
        # See also lldbtest.TestBase.setUpClass(cls).
        if len(testdirs) == 1 and os.path.basename(testdirs[0]) == 'test':
            os.environ["LLDB_TEST"] = os.path.join(rdir, 'test')
        else:
            os.environ["LLDB_TEST"] = rdir
    else:
        os.environ["LLDB_TEST"] = scriptPath
    pluginPath = os.path.join(scriptPath, 'plugins')

    # Append script dir and plugin dir to the sys.path.
    sys.path.append(scriptPath)
    sys.path.append(pluginPath)
    
    global ignore

    # The '-i' option is used to skip looking for lldb.py in the build tree.
    if ignore:
        return
        
    base = os.path.abspath(os.path.join(scriptPath, os.pardir))
    dbgPath = os.path.join(base, 'build', 'Debug', 'LLDB.framework',
                           'Resources', 'Python')
    relPath = os.path.join(base, 'build', 'Release', 'LLDB.framework',
                           'Resources', 'Python')
    baiPath = os.path.join(base, 'build', 'BuildAndIntegration',
                           'LLDB.framework', 'Resources', 'Python')

    lldbPath = None
    if os.path.isfile(os.path.join(dbgPath, 'lldb.py')):
        lldbPath = dbgPath
    elif os.path.isfile(os.path.join(relPath, 'lldb.py')):
        lldbPath = relPath
    elif os.path.isfile(os.path.join(baiPath, 'lldb.py')):
        lldbPath = baiPath

    if not lldbPath:
        print 'This script requires lldb.py to be in either ' + dbgPath + ',',
        print relPath + ', or ' + baiPath
        sys.exit(-1)

    # This is to locate the lldb.py module.  Insert it right after sys.path[0].
    sys.path[1:1] = [lldbPath]


def doDelay(delta):
    """Delaying startup for delta-seconds to facilitate debugger attachment."""
    def alarm_handler(*args):
        raise Exception("timeout")

    signal.signal(signal.SIGALRM, alarm_handler)
    signal.alarm(delta)
    sys.stdout.write("pid=%d\n" % os.getpid())
    sys.stdout.write("Enter RET to proceed (or timeout after %d seconds):" %
                     delta)
    sys.stdout.flush()
    try:
        text = sys.stdin.readline()
    except:
        text = ""
    signal.alarm(0)
    sys.stdout.write("proceeding...\n")
    pass


def visit(prefix, dir, names):
    """Visitor function for os.path.walk(path, visit, arg)."""

    global suite
    global regexp
    global filterspec
    global fs4all

    for name in names:
        if os.path.isdir(os.path.join(dir, name)):
            continue

        if '.py' == os.path.splitext(name)[1] and name.startswith(prefix):
            # Try to match the regexp pattern, if specified.
            if regexp:
                import re
                if re.search(regexp, name):
                    #print "Filename: '%s' matches pattern: '%s'" % (name, regexp)
                    pass
                else:
                    #print "Filename: '%s' does not match pattern: '%s'" % (name, regexp)
                    continue

            # We found a match for our test.  Add it to the suite.

            # Update the sys.path first.
            if not sys.path.count(dir):
                sys.path.insert(0, dir)
            base = os.path.splitext(name)[0]

            # Thoroughly check the filterspec against the base module and admit
            # the (base, filterspec) combination only when it makes sense.
            if filterspec:
                # Optimistically set the flag to True.
                filtered = True
                module = __import__(base)
                parts = filterspec.split('.')
                obj = module
                for part in parts:
                    try:
                        parent, obj = obj, getattr(obj, part)
                    except AttributeError:
                        # The filterspec has failed.
                        filtered = False
                        break
                # Forgo this module if the (base, filterspec) combo is invalid
                # and the '-g' option is present.
                if fs4all and not filtered:
                    continue
                
            # Add either the filtered test case or the entire test class.
            if filterspec and filtered:
                suite.addTests(
                    unittest2.defaultTestLoader.loadTestsFromName(filterspec, module))
            else:
                # A simple case of just the module name.  Also the failover case
                # from the filterspec branch when the (base, filterspec) combo
                # doesn't make sense.
                suite.addTests(unittest2.defaultTestLoader.loadTestsFromName(base))


def lldbLoggings():
    """Check and do lldb loggings if necessary."""

    # Turn on logging for debugging purposes if ${LLDB_LOG} environment variable is
    # defined.  Use ${LLDB_LOG} to specify the log file.
    ci = lldb.DBG.GetCommandInterpreter()
    res = lldb.SBCommandReturnObject()
    if ("LLDB_LOG" in os.environ):
        if ("LLDB_LOG_OPTION" in os.environ):
            lldb_log_option = os.environ["LLDB_LOG_OPTION"]
        else:
            lldb_log_option = "event process"
        ci.HandleCommand(
            "log enable -f " + os.environ["LLDB_LOG"] + " lldb " + lldb_log_option,
            res)
        if not res.Succeeded():
            raise Exception('log enable failed (check LLDB_LOG env variable.')
    # Ditto for gdb-remote logging if ${GDB_REMOTE_LOG} environment variable is defined.
    # Use ${GDB_REMOTE_LOG} to specify the log file.
    if ("GDB_REMOTE_LOG" in os.environ):
        if ("GDB_REMOTE_LOG_OPTION" in os.environ):
            gdb_remote_log_option = os.environ["GDB_REMOTE_LOG_OPTION"]
        else:
            gdb_remote_log_option = "packets"
        ci.HandleCommand(
            "log enable -f " + os.environ["GDB_REMOTE_LOG"] + " process.gdb-remote "
            + gdb_remote_log_option,
            res)
        if not res.Succeeded():
            raise Exception('log enable failed (check GDB_REMOTE_LOG env variable.')


############################################
#                                          #
# Execution of the test driver starts here #
#                                          #
############################################

#
# Start the actions by first parsing the options while setting up the test
# directories, followed by setting up the search paths for lldb utilities;
# then, we walk the directory trees and collect the tests into our test suite.
#
parseOptionsAndInitTestdirs()
setupSysPath()

#
# If '-d' is specified, do a delay of 10 seconds for the debugger to attach.
#
if delay:
    doDelay(10)

#
# If '-l' is specified, do not skip the long running tests.
if not skipLongRunningTest:
    os.environ["LLDB_SKIP_LONG_RUNNING_TEST"] = "NO"

#
# Walk through the testdirs while collecting tests.
#
for testdir in testdirs:
    os.path.walk(testdir, visit, 'Test')

#
# Now that we have loaded all the test cases, run the whole test suite.
#

# For the time being, let's bracket the test runner within the
# lldb.SBDebugger.Initialize()/Terminate() pair.
import lldb, atexit
# Update: the act of importing lldb now executes lldb.SBDebugger.Initialize(),
# there's no need to call it a second time.
#lldb.SBDebugger.Initialize()
atexit.register(lambda: lldb.SBDebugger.Terminate())

# Create a singleton SBDebugger in the lldb namespace.
lldb.DBG = lldb.SBDebugger.Create()

# Turn on lldb loggings if necessary.
lldbLoggings()

# Install the control-c handler.
unittest2.signals.installHandler()

# Now get a timestamp string and export it as LLDB_TIMESTAMP environment var.
# This will be useful when/if we want to dump the session info of individual
# test cases later on.
#
# See also TestBase.dumpSessionInfo() in lldbtest.py.
import datetime
raw_timestamp = str(datetime.datetime.today())
usec_position = raw_timestamp.rfind('.')
if usec_position != -1:
    raw_timestamp = raw_timestamp[:usec_position]
os.environ["LLDB_TIMESTAMP"] = raw_timestamp.replace(' ', '-')

#
# Invoke the default TextTestRunner to run the test suite, possibly iterating
# over different configurations.
#

iterArchs = False
iterCompilers = False

from types import *
if "archs" in config:
    archs = config["archs"]
    if type(archs) is ListType and len(archs) >= 1:
        iterArchs = True
if "compilers" in config:
    compilers = config["compilers"]
    if type(compilers) is ListType and len(compilers) >= 1:
        iterCompilers = True

# Make a shallow copy of sys.path, we need to manipulate the search paths later.
# This is only necessary if we are relocated and with different configurations.
if rdir and (iterArchs or iterCompilers):
    old_sys_path = sys.path[:]
    old_stderr = sys.stderr
    old_stdout = sys.stdout
    new_stderr = None
    new_stdout = None

for ia in range(len(archs) if iterArchs else 1):
    archConfig = ""
    if iterArchs:
        os.environ["ARCH"] = archs[ia]
        archConfig = "arch=%s" % archs[ia]
    for ic in range(len(compilers) if iterCompilers else 1):
        if iterCompilers:
            os.environ["CC"] = compilers[ic]
            configString = "%s compiler=%s" % (archConfig, compilers[ic])
        else:
            configString = archConfig

        if iterArchs or iterCompilers:
            # If we specified a relocated directory to run the test suite, do
            # the extra housekeeping to copy the testdirs to a configStringified
            # directory and to update sys.path before invoking the test runner.
            # The purpose is to separate the configuration-specific directories
            # from each other.
            if rdir:
                from string import maketrans
                from shutil import copytree, ignore_patterns

                # Translate ' ' to '-' for dir name.
                tbl = maketrans(' ', '-')
                configPostfix = configString.translate(tbl)
                newrdir = "%s.%s" % (rdir, configPostfix)

                # Copy the tree to a new directory with postfix name configPostfix.
                copytree(rdir, newrdir, ignore=ignore_patterns('*.pyc', '*.o', '*.d'))

                # Check whether we need to split stderr/stdout into configuration
                # specific files.
                if old_stderr.name != '<stderr>' and config.get('split_stderr'):
                    if new_stderr:
                        new_stderr.close()
                    new_stderr = open("%s.%s" % (old_stderr.name, configPostfix), "w")
                    sys.stderr = new_stderr
                if old_stdout.name != '<stdout>' and config.get('split_stdout'):
                    if new_stdout:
                        new_stdout.close()
                    new_stdout = open("%s.%s" % (old_stdout.name, configPostfix), "w")
                    sys.stdout = new_stdout

                # Update the LLDB_TEST environment variable to reflect new top
                # level test directory.
                #
                # See also lldbtest.TestBase.setUpClass(cls).
                if len(testdirs) == 1 and os.path.basename(testdirs[0]) == 'test':
                    os.environ["LLDB_TEST"] = os.path.join(newrdir, 'test')
                else:
                    os.environ["LLDB_TEST"] = newrdir

                # And update the Python search paths for modules.
                sys.path = [x.replace(rdir, newrdir, 1) for x in old_sys_path]

            # Output the configuration.
            sys.stderr.write("\nConfiguration: " + configString + "\n")

        #print "sys.stderr name is", sys.stderr.name
        #print "sys.stdout name is", sys.stdout.name

        # First, write out the number of collected test cases.
        sys.stderr.write(separator + "\n")
        sys.stderr.write("Collected %d test%s\n\n"
                         % (suite.countTestCases(),
                            suite.countTestCases() != 1 and "s" or ""))

        # Invoke the test runner.
        class LLDBTestResult(unittest2.TextTestResult):
            """
            Enforce a singleton pattern to allow inspection of test progress.
            """
            __singleton__ = None

            def __init__(self, *args):
                if LLDBTestResult.__singleton__:
                    raise "LLDBTestResult instantiated more than once"
                super(LLDBTestResult, self).__init__(*args)
                LLDBTestResult.__singleton__ = self
                # Now put this singleton into the lldb module namespace.
                lldb.test_result = self

            def addError(self, test, err):
                super(LLDBTestResult, self).addError(test, err)
                method = getattr(test, "markError", None)
                if method:
                    method()

            def addFailure(self, test, err):
                super(LLDBTestResult, self).addFailure(test, err)
                method = getattr(test, "markFailure", None)
                if method:
                    method()

        result = unittest2.TextTestRunner(stream=sys.stderr, verbosity=verbose,
                                          resultclass=LLDBTestResult).run(suite)
        

# Terminate the test suite if ${LLDB_TESTSUITE_FORCE_FINISH} is defined.
# This should not be necessary now.
if ("LLDB_TESTSUITE_FORCE_FINISH" in os.environ):
    import subprocess
    print "Terminating Test suite..."
    subprocess.Popen(["/bin/sh", "-c", "kill %s; exit 0" % (os.getpid())])

# Exiting.
sys.exit(not result.wasSuccessful)
