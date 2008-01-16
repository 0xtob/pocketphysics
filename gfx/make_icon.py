#!/usr/bin/python

import sys,os

rgbfile = sys.argv[1][:-3]+"rgb"
rawfile = sys.argv[1][:-3]+"raw"
os.popen("convert "+sys.argv[1]+" "+rgbfile)
os.popen("./rgb2bin "+rgbfile)
os.popen("cp "+rawfile+" ../arm9/data")
