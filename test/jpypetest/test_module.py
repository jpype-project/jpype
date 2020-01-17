# Tests for module functionality including failures that cannot
# be triggered in normal operations
import _jpype
import unittest 
import pickle
import os
import subprocess
import sys

# We don't want JPype loaded during this operation
if __name__!="subprocess":
    import jpype

# Utility to run a subprocess and get the result
def subrun(function, *args):
    """ Utility function to launch a subprocess and get the result. """
    os.environ['PYTHONPATH'] = os.getcwd()
    pickle.dump(args, open("input.pic",'wb'))
    child = subprocess.Popen([sys.executable, "-"],stdin=subprocess.PIPE)
    prog= []
    prog.append('with open(r"%s","r") as fd:'%os.path.abspath(__file__))
    prog.append('  contents=fd.read()')
    prog.append('__name__="subprocess"')
    prog.append('exec(contents)')
    prog.append('args=pickle.load(open("input.pic","rb"))')
    prog.append('ex=None')
    prog.append('ret=None')
    prog.append('try:')
    prog.append('  ret=%s(*args)'%function.__name__)
    prog.append('except Exception as ex1:')
    prog.append('  ex=ex1')
    prog.append('pickle.dump([ret,ex], open("output.pic","wb"))')
    child.communicate(input=bytes("\n".join(prog), 'utf-8'))
    [ret,ex]=pickle.load(open("output.pic","rb"))
    child.wait()
    os.remove("input.pic")
    os.remove("output.pic")
    if ex:
        raise ex
    return ret


##############################################################################
# Test methods

def runStartup(path):
    _jpype.startup(path, tuple(), False, False)
    return True

def runStartupBadArgs(path):
    _jpype.startup(path)

def runNoMethodDoc(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__doc__  # RuntimeError

def runNoMethodAnnotation(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__annotations__  # RuntimeError

def runNoMethodCode(path):
    _jpype.startup(path, tuple(), False, False)
    cls = _jpype.PyJPClass("java.lang.String")
    methods = cls.getClassMethods()
    methods[0].__code__  # RuntimeError

def runValueEntry():
    # fails as no JVM is running yet
    _jpype.PyJPValue()

def runShutdown():
    import jpype
    jpype.startJVM(convertStrings=False)
    jpype.shutdownJVM()


##############################################################################

class TestModule(unittest.TestCase):
    def setUp(self):
        self.path = jpype.getDefaultJVMPath()
    def testStartup(self):
        self.assertTrue(subrun(runStartup, self.path))
    def testStartupBadArgs(self):
        with self.assertRaises(TypeError):
          subrun(runStartupBadArgs, self.path)
    def testNoMethodDoc(self):
        with self.assertRaises(RuntimeError):
          subrun(runNoMethodDoc, self.path)
    def testNoMethodAnnotation(self):
        with self.assertRaises(RuntimeError):
          subrun(runNoMethodAnnotation, self.path)
    def testNoMethodCode(self):
        with self.assertRaises(RuntimeError):
          subrun(runNoMethodCode, self.path)
    def testValueEntry(self):
        with self.assertRaises(RuntimeError):
            subrun(runValueEntry)
    def testShutdown(self):
        subrun(runShutdown)
