# Tests for beans
import jpype
import unittest 
import pickle
import os
import subprocess
import sys
import inspect

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

def assertTrue(logic, *args):
    if not logic:
        frame = sys._getframe(1)
        lines = inspect.getsourcelines(frame)
        raise RuntimeError(lines[0][frame.f_lineno-1])

def assertEqual(value1, value2, *args):
    if value1!=value2:
        frame = sys._getframe(1)
        lines = inspect.getsourcelines(frame)
        raise RuntimeError("%s!=%s at %s"%(value1,value2,lines[0][frame.f_lineno-1]))

def runBeans():
    import jpype
    import jpype.beans
    jpype.startJVM(classpath=os.path.abspath("test/classes"), convertStrings=False)
    cls = jpype.JClass("jpype/properties/TestBean")
    obj = cls()
    assertTrue(isinstance(cls.__dict__['propertyMember'], property))
    assertTrue(isinstance(cls.__dict__['readOnly'], property))
    assertTrue(isinstance(cls.__dict__['writeOnly'], property))
    assertTrue(isinstance(cls.__dict__['with_'], property))

    # Test member
    obj.propertyMember = "q"
    assertEqual(obj.propertyMember, "q")

    # Test keyword property
    obj.with_="a"
    assertEqual(obj.with_,"a")
    assertEqual(obj.m5,"a")

    # Test readonly
    obj.m3="b"
    assertEqual(obj.readOnly,"b")
    try:
        obj.readOnly="c"
        raise RuntimeError("wrote to readonly property")
    except AttributeError:
        pass

    # Test writeonly
    obj.writeOnly="c"
    assertEqual(obj.m4,"c")
    try:
        print(obj.writeOnly)
        raise RuntimeError("read from writeonly property")
    except AttributeError:
        pass
    try:
        cls.__dict__['failure1']
        raise TypeError("got property failure1")
    except KeyError:
        pass
    try:
        cls.__dict__['failure2']
        raise TypeError("got property failure2")
    except KeyError:
        pass
    return True


##############################################################################

class BeansTest(unittest.TestCase):
    def setUp(self):
        self.path = jpype.getDefaultJVMPath()
    def testBeans(self):
        self.assertTrue(subrun(runBeans))
