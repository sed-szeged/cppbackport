#!/usr/bin/python3

from enum import Enum
import argparse
import subprocess
import os
import platform
import shutil
import sys
import difflib
import filecmp
import time

class TestResult(Enum):
  Success = 1
  FailedRefDiff = 2
  FailedBackportTool = 3
  FailedCompError = 4
  FailedRuntimeError = 5
  Undecided = 6

class Os(Enum):
  Windows = 1
  Linux = 2
  Unsupported = 10

class Test:
  """Holds the datas of a test."""
  def __init__(self, path, command):
    self.path = path
    self.command = command
    self.result = TestResult.Undecided

  def __str__(self):
    """Converts the object to a string."""
    return "Path: " + self.path + " Command:" + self.command + " Result: " + self.result.name

class TestManager:
  """Manages the available tests."""
  tool = '' # is based on command line argument
  compilers = []
  subprocess_flags = 0

  def __init__(self):
    self.testsuite = []

  def listTests(self):
    """Prints the available tests cases in the object."""
    for test in self.testsuite:
      print(test)

  def buildTestSuite(self):
    """Reads and creates the tests cases from the current directory."""
    for root, dirs, files in os.walk('.'):
      for name in files:
        path = os.path.join(root, name)
        if "cpp" in os.path.splitext(path)[1] and TestManager.isTestFile(path):
          self.addTest(TestManager.parseTest(path))

  def addTest(self, test):
    """Appends the given test case to the list."""
    self.testsuite.append(test)

  def runTests(self):
    """Runs all test cases."""
    for test in self.testsuite:
      print('Run test: ' + test.path + " ...", end="")
      sys.stdout.flush()
      self.runTest(test)
    
  def removeTempDirs(self):
    """Removes temp dirs."""
    for root, dirs, files in os.walk('.'):
        for name in dirs:
          if name == "temp":
            shutil.rmtree(os.path.join(root, name))
  
  def summarizeResults(self):
    """Prints the result of the tests cases."""
    nOfTests = len(self.testsuite)
    nOfSucTests = len(list(filter(lambda x: x.result == TestResult.Success, self.testsuite)))

    print("SUMMARY:")
    print("Number of tests:", nOfTests)
    print("Number of successful tests:", nOfSucTests)
    if nOfTests == nOfSucTests:
      return

    print("Failed tests:")
    for test in filter(lambda x: x.result != TestResult.Success, self.testsuite):
      print("Test:", test.path, "Reason:", test.result)


  def runTest(self, test):
    runner = TestRunner(test)
    runner.run()

  def parseTest(path):
    """Creates a Test object from a file."""
    with open(path) as f:
      for line in f:
        if line.startswith("// RUN:"):
          command = line.replace("// RUN:", "").rstrip('\n').replace("backport", TestManager.tool)
        else:
          break
    return Test(path, command)

  def isTestFile(path):
    """Returns true if a file contains the // RUN: line in the first line."""
    with open(path) as f:
      line = f.readline()
      if line.startswith("// RUN:"):
        return True
    return False

class TestRunner:
  curros = Os.Unsupported

  """Runs the given test case object and checks the result of a run."""
  def __init__(self, test):
    self.test = test
    self.oldpath = os.getcwd()
    spath = os.path.split(test.path)
    self.testdir = spath[0]
    self.tmpdir = os.path.join(spath[0], "temp")
    logfile = test.path.replace("\\", "_")
    logfile = logfile.replace("/", "_")
    self.errorfile = os.path.join(logfile + '.log')
    self.filename = spath[1]

  def prepareTest(self):
    """Prepares the necessary dependencies of the test cases."""
    self.logdir = os.path.join(self.oldpath, "log")
    if not os.path.exists(self.logdir):
      os.mkdir(self.logdir)

    self.err = open(os.path.join(self.logdir, self.errorfile), 'w')

  def run(self):
    """Runs the stored test."""
    self.prepareTest()
    self.runTest()
    self.cleanup()

  def runTest(self):
    """Runs the required processes to determine the result of the test cases."""

    # call backport
    os.chdir(self.testdir)
    res = subprocess.call(self.test.command, stdout=self.err, stderr=self.err, shell=True, creationflags=TestManager.subprocess_flags)
    if res:
      self.test.result = TestResult.FailedBackportTool
      print("FAILED (Backport tool encountered an error)")
      os.chdir(self.oldpath)
      return

    os.chdir("temp")

    # call compilers
    if TestManager.compilers is not None:
      for comp in TestManager.compilers:
        res = subprocess.call(comp + " " + self.filename, stdout=self.err, stderr=self.err, shell=True, creationflags=TestManager.subprocess_flags)
        if res:
          print('FAILED (Compile error)')
          self.test.result = TestResult.FailedCompError
          os.chdir(self.oldpath)
          return

        filename = os.path.splitext(self.filename)[0] + ".exe" if TestRunner.curros == Os.Windows else "./a.out"
        res = subprocess.call(filename, stdout=self.err, stderr=self.err, shell=True, creationflags=TestManager.subprocess_flags)
        if res:
          print('FAILED (Runtime error)')
          self.test.result = TestResult.FailedRuntimeError
          os.chdir(self.oldpath)
          return

    os.chdir(self.oldpath)
    self.test.result = TestResult.Success
    print("SUCCESS")

  def cleanup(self):
    """Removes the additionally created directories."""
    self.err.close()
    if self.test.result == TestResult.Success:
      os.remove(os.path.join(self.logdir, self.errorfile))


def main():
  curros = detectOs()
  if curros == Os.Unsupported:
    raise RuntimeError("Unsupported operating system.")
  elif curros == Os.Windows:
    applyWindowsSpecificSettings()

  args = initArgParser()

  TestRunner.curros = curros
  testmanager = TestManager()

  if not checkBackPortTool(args.backport_tool, curros):
    raise RuntimeError("Couldn't found backport tool")

  TestManager.compilers = args.selected_compilers
  testmanager.removeTempDirs()
  testmanager.buildTestSuite()

  if args.list_tests:
    testmanager.listTests()
    return

  testmanager.runTests()
  testmanager.summarizeResults()

def initArgParser():
  parser = argparse.ArgumentParser(description='Backport test runner runs regtests of backport tool')
  parser.add_argument('-b', '--backport-tool', help='Path to backport tool')
  parser.add_argument('-c', '--selected-compilers', nargs='*', help='Selected compiler for testing output of tests')
  parser.add_argument('-l', '--list-tests', action='store_true', help='Lists tests')
  return parser.parse_args()

def checkBackPortTool(path, curros):
  if curros == Os.Linux:
    status, result = subprocess.getstatusoutput("backport")
    if status == 1:
      TestManager.tool = "backport"
      return True

  if not path or not os.path.exists(path):
    return False

  appName = "backport.exe" if curros == Os.Windows else "backport"
  if not os.path.exists(os.path.join(path, appName)):
    return False

  TestManager.tool = os.path.join(path, appName)
  return True

def applyWindowsSpecificSettings():
  # Don't display the Windows GPF dialog if the invoked program dies.
  # See comp.os.ms-windows.programmer.win32
  #  How to suppress crash notification dialog?, Jan 14,2004 -
  #     Raymond Chen's response [1]
  import ctypes
  SEM_NOGPFAULTERRORBOX = 0x0002 # From MSDN
  ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX);
  TestManager.subprocess_flags = 0x08000000

def detectOs():
  if platform.system() == 'Windows':
    return Os.Windows
  elif platform.system() == 'Linux':
    return Os.Linux
  else:
    return Os.Unsupported

if __name__ == '__main__':
  main()
