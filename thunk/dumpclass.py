import glob
import os, array

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
    chunk = 8
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
    print("jsize %s_size = %d;" % (cname, sz), file=fout)
    print(file=fout)
    f.close()

with open("../native/common/jp_thunk.cpp","w+") as fimpl, open("../native/common/include/jp_thunk.h","w+") as fheader:
    print("#ifndef JP_THUNK_H__", file=fheader)
    print("#define JP_THUNK_H__", file=fheader)
    print("#include <jpype.h>", file=fheader)
    print("namespace JPThunk {", file=fheader)

    print("#include <jpype.h>", file=fimpl)
    print("namespace JPThunk {", file=fimpl)
    for filename in glob.iglob("classes/**/*.class", recursive=True):
        name=filename[8:-6].replace('/','_')
        outputClass(filename, name, fimpl)
        print("extern jbyte %s[];"%name, file=fheader)
        print("extern jsize %s_size;"%name, file=fheader)
    print("}", file=fheader)
    print("#endif", file=fheader)
    print("}", file=fimpl)
