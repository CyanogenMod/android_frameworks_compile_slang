#!/usr/bin/python2.4
#
# Copyright 2010 Google Inc. All Rights Reserved.

"""RenderScript Compiler Test.

Runs subdirectories of tests for the RenderScript compiler.
"""

import filecmp
import glob
import os
import shutil
import subprocess
import sys

__author__ = 'Android'


class Options(object):
  def __init__(self):
    pass
  verbose = 0


def CompareFiles(filename):
  actual = filename
  expect = filename + '.expect'
  return filecmp.cmp(actual, expect, False)


def ExecTest(dirname):
  """Executes an llvm-rs-cc test from dirname."""
  passed = True

  if Options.verbose != 0:
    print 'Testing %s' % dirname

  os.chdir(dirname)
  stdout_file = open('stdout.txt', 'w')
  stderr_file = open('stderr.txt', 'w')

  cmd_string = ('../../../../../out/host/linux-x86/bin/llvm-rs-cc '
                '-o tmp/ -p tmp/ '
                '-I ../../../../../frameworks/base/libs/rs/scriptc/')
  base_args = cmd_string.split()
  rs_files = glob.glob('*.rs')
  args = base_args + rs_files

  # Execute the command and check the resulting shell return value.
  # All tests that are expected to FAIL have directory names that
  # start with 'F_'. Other tests that are expected to PASS have
  # directory names that start with 'P_'.
  ret = subprocess.call(args, stdout=stdout_file, stderr=stderr_file)
  if dirname[0:2] == 'F_':
    if ret == 0:
      passed = False
      if Options.verbose:
        print 'Command passed on invalid input'
  elif dirname[0:2] == 'P_':
    if ret != 0:
      passed = False
      if Options.verbose:
        print 'Command failed on valid input'
  else:
    passed = False
    if Options.verbose:
      print 'Test Directory name should start with an F or a P'

  stdout_file.flush()
  stderr_file.flush()
  stdout_file.close()
  stderr_file.close()

  if not CompareFiles('stdout.txt'):
    passed = False
    if Options.verbose:
      print 'stdout is different'
  if not CompareFiles('stderr.txt'):
    passed = False
    if Options.verbose:
      print 'stderr is different'

  os.remove('stdout.txt')
  os.remove('stderr.txt')
  shutil.rmtree('tmp/')

  os.chdir('..')
  return passed


def main():
  passed = 0
  failed = 0

  for arg in sys.argv[1:]:
    if arg == '-v':
      Options.verbose = 1
    else:
      print >> sys.stderr, 'Invalid argument %s' % arg
      return 1

  files = os.listdir('.')
  for f in files:
    if os.path.isdir(f):
      if ExecTest(f):
        passed += 1
      else:
        failed += 1

  print 'Tests Passed: %d\n' % passed,
  print 'Tests Failed: %d\n' % failed,

  return failed != 0


if __name__ == '__main__':
  sys.exit(main())
