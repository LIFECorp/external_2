#!/usr/bin/python
#
# Copyright (C) 2013 Mediatek Inc.Co
#
# This script is developed for user to package icu5.1 quickly
#
# Author: WCD/OSS6/AF3 David Wu (yang.wu@mediatek.com)

import os
import getopt
import sys
import subprocess

def DumpHelpMenu():
  print "Usage:"
  print "  OneShot.py [-n|--new] [-h|--help]"
  sys.exit(1)

def main():
  print "=============================================="
  print "====== One Shot ICU Package System V5.1 ======"
  print "======          WCD/OSS6/AF3            ======"
  print "=============================================="
  global ICU_BUILD_PATH
  global ICU_BUILD_ROOT
  global ICU_BUILD_DATA
  ICU_BUILD_ROOT = os.getcwd()
  ICU_BUILD_PATH = os.path.join(ICU_BUILD_ROOT, "icuBuid")
  ICU_BUILD_DATA = os.path.join(ICU_BUILD_ROOT, "stubdata")
  print "Locate the temporary directory: %s" % ICU_BUILD_PATH

  show_help = False
  new_build = True
  debug     = False

  try:
    opts, args = getopt.getopt(sys.argv[1:], "nrdh", ["new", "rebuild", "debug", "help"])
  except getopt.error:
    DumpHelpMenu()

  for opt, _ in opts:
    if opt in ("-h", "--help"):
      show_help = True
    elif opt in ("-n", "--new"):
      print "Start new Build"
      new_build = True
    elif opt in ("-r", "--rebuild"):
      print "Start re Build"
      new_build = False

    if opt in ("-d", "--debug"):
      print "Debug Mode Enabled"
      debug = True

  if args:
    show_help = True

  if show_help:
    DumpHelpMenu()

  if (new_build):
    if os.path.exists(ICU_BUILD_PATH) and os.path.isdir(ICU_BUILD_PATH):
      try:
        os.popen('rm -fr icuBuid')
      except:
        print "[Error] Can not remove directory %s" % ICU_BUILD_PATH

    try:
      os.mkdir(ICU_BUILD_PATH)
      os.chdir(ICU_BUILD_PATH)
      print "Temporary directory has been created and current directory is %s" % os.getcwd()
      p = subprocess.Popen(["./../runConfigureICU", "Linux"])
      p.wait()
    except:
      print "[Error] Can not operate on folder %s" % ICU_BUILD_PATH
      pass

  try:
    os.chdir(ICU_BUILD_PATH)
    p = subprocess.Popen(["make", "-j2"])
    p.wait()
  except:
    print "[Error] Error during making resource and current path is %s" % os.getcwd()
    pass

  try:
    os.popen('cp %s/data/out/tmp/icudt51l.dat %s/icudt51l-all.dat' % (ICU_BUILD_PATH, ICU_BUILD_DATA))
    os.chdir(ICU_BUILD_DATA)
    if not debug:
      os.popen('rm -fr %s' % ICU_BUILD_PATH)
    p = subprocess.Popen(["./icu_dat_generator.py"])
    p.wait()
    os.popen('rm icudt51l-all.dat')
    os.popen('mv icudt51l-default.dat icudt51l-all.dat')
  except:
    print "[Error] Can not remove directory %s" % ICU_BUILD_PATH

  print "Output file is under: %s/icudt51l-all.dat" % ICU_BUILD_DATA
  print "=============================================="
  print "======             Good Bye             ======"
  print "=============================================="

if __name__ == "__main__":
  main()
