import os
import sys
import glob
import subprocess

if __name__ == "__main__":
    sep = ':'
    if sys.platform == "cygwin" or sys.platform == "win32":
        sep = ';'
 
    home = os.path.dirname(__file__)
    args = []
    args.append('java')
    args.append('-cp')
    l =  sep.join(glob.glob(os.path.join(home,"..","lib","*.jar")))
    args.append(l)
    args.append("org.apache.tools.ant.launch.Launcher")
    args.extend(sys.argv[1:])
    subprocess.run(args)
