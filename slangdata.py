#!/usr/bin/env python

import os, sys

def print_header(var_name):
  sys.stdout.write("""
#ifdef __APPLE_CC__
/*\n\
 * The mid-2007 version of gcc that ships with Macs requires a\n\
 * comma on the .section line, but the rest of the world thinks\n\
 * that's a syntax error. It also wants globals to be explicitly\n\
 * prefixed with \"_\" as opposed to modern gccs that do the\n\
 * prefixing for you.\n\
 */\n\
.globl _%s\n\
  .section .rodata,\n\
  .align 8\n\
_%s:\n\
#else\n\
.globl %s\n\
  .section .rodata\n\
  .align 8\n\
%s:\n\
#endif\n\
""" % (var_name, var_name, var_name, var_name))

def file2asm(var_name):
  print_header(var_name)

  input_size = 0
  col = 0
  while True:
    buf = sys.stdin.read(1024)
    if len(buf) <= 0:
      break
    input_size += len(buf)
    for c in buf:
      if col == 0:
        sys.stdout.write(".byte ")
      sys.stdout.write("0x%02x" % ord(c))
      col += 1
      if col == 16:
        sys.stdout.write("\n")
        col = 0
      elif col % 4 == 0:
        sys.stdout.write(", ")
      else:
        sys.stdout.write(",")
  # always ends with 0x0 (can fix assembler warnings)
  if col != 0:
    sys.stdout.write("0x00")
    sys.stdout.write("\n")

  # encode file size
  print_header(var_name + "_size")
  sys.stdout.write("  .long %d\n" % input_size)

def main(argv):
  if len(argv) < 2:
    print "usage: %s <name>" % argv[0]
    return 1

  file2asm(argv[1])

if __name__ == '__main__':
  main(sys.argv)
