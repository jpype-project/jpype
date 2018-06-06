# -*- coding: utf-8 -*-
from __future__ import print_function
import os
import subprocess
import distutils.cmd
import distutils.log
import array
import sys

if (sys.version_info < (3, 0)):
    import string
    def translate(s, cfrom, cto):
        return s.translate(string.maketrans(cfrom, cto))
else:
    def translate(s, cfrom, cto): 
        return s.translate(str.maketrans(cfrom, cto))

import fnmatch

# Python2/3 don't agree on how glob should work
def _glob(directory,pattern):
    out = []
    for root, dirnames, filenames in os.walk(directory):
        for filename in fnmatch.filter(filenames, pattern):
            out.append(os.path.join(root,filename))
    return out

def output(fout, l) :
    print("    ", file=fout, end="")
    line = []
    buffer = array.array("B")
    buffer.fromstring(l)
    for i in buffer :
        line.append( "(jbyte)0x%02X" % i )
    print(",".join(line), file=fout, end="")

def outputClass(srcfname, cname, fout):
    f = open(srcfname, "rb");
    chunk = 16
    print("jbyte %s[] = {" % cname, file=fout)
    sz = 0
    while True:
        l = f.read(chunk)
        if len(l)==0:
            break
        if sz>0:
            print(",", file=fout)
        output(fout, l);
        sz += len(l);

    print(file=fout)
    print("};", file=fout)
    print("int %s_size = %d;" % (cname, sz), file=fout)
    print(file=fout)
    f.close()

def mkFileDir(filename):
  if not os.path.exists(os.path.dirname(filename)):
    try:
        os.makedirs(os.path.dirname(filename))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

def createThunks(input_dir, output_src, output_header, namespace="Thunk"):
    mkFileDir(output_src)
    mkFileDir(output_header)

    # Write the header
    with open(output_header,"w+") as fheader:
        sz=len(input_dir)
        guard=translate(output_header.upper(),'/\\.','___')
        print("#ifndef %s"%guard, file=fheader)
        print("#define %s"%guard, file=fheader)
        print("#include <jpype.h>", file=fheader)
        print("namespace %s {"%namespace, file=fheader)
        for filename in _glob(input_dir, "*.class"):
            name=translate(filename,'/\\.','___')[sz:-6]
            print("extern jbyte %s[];"%name, file=fheader)
            print("extern int %s_size;"%name, file=fheader)
        for filename in _glob(input_dir, "*.jar"):
            name=translate(filename,'/\\.','___')[sz:-4]
            print("extern jbyte %s[];"%name, file=fheader)
            print("extern int %s_size;"%name, file=fheader)
        print("}", file=fheader)
        print("#endif", file=fheader)

    # Write the body
    with open(output_src,"w+") as fimpl:
        sz=len(input_dir)
        print("#include <jp_thunk.h>", file=fimpl)
        print("namespace %s {"%namespace, file=fimpl)
        for filename in _glob(input_dir, "*.class"):
            print("  including thunk %s"%filename)
            name=translate(filename,'/\\.','___')[sz:-6]
            outputClass(filename, name, fimpl)
        for filename in _glob(input_dir, "*.jar"):
            print("  including thunk %s"%filename)
            name=translate(filename,'/\\.','___')[sz:-4]
            outputClass(filename, name, fimpl)
        print("}", file=fimpl)

class BuildThunkCommand(distutils.cmd.Command):
  """A custom command to create thunk file."""

  description = 'build dynamic code thunks'
  user_options = [
  ]

  def initialize_options(self):
    """Set default values for options."""
    pass

  def finalize_options(self):
    """Post-process options."""
    pass

  def run(self):
    """Run command."""
    self.announce(
        'Building thunks',
        level=distutils.log.INFO)
    # run short circuit logic here
    srcDir = os.path.join("build","lib")
    destBody = os.path.join("build","src","jp_thunk.cpp")
    destHeader = os.path.join("build","src","jp_thunk.h")

    if os.path.isfile(destBody):
        t1=os.path.getctime(destBody)
        update =False
        for filename in _glob(srcDir, "*.class"):
            if t1<os.path.getctime(filename):
                update=True
        if not update:
            self.announce(
                'Skip build thunks',
                level=distutils.log.INFO)
            return

    # do the build
    createThunks(
            srcDir,
            destBody, 
            destHeader, 
            namespace="JPThunk")

