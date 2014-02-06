#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************
from jpype import JException, java, JavaException, JProxy, JPackage
import common
import traceback

def throwIOException() :
	raise java.io.IOException.PYEXC("Test throw")

def throwByJavaException() :
	JPackage('jpype').exc.ExceptionTest.throwIOException()

class ExceptionTestCase(common.JPypeTestCase) :
	def testExceptionThrown(self) :
		try :
			self.jpype.exc.ExceptionTest.throwRuntime()
			assert False
		except JavaException, ex :
			print 'Caught a Java exception ...'
			if ex.javaClass() is java.lang.RuntimeException :
				print "Caught the exception", ex.message()
				print ex.stacktrace()
			else:
				assert False
		except Exception, ex:
			print ex.__class__, isinstance(ex, JavaException)
			print ex.__class__.__bases__[0].__bases__[0].__bases__
			print JavaException
			assert False
			
		print 'if here, everything is fine'
			
	def testExceptionByJavaClass(self) :
		try :
			self.jpype.exc.ExceptionTest.throwRuntime()
			assert False
		except JException(java.lang.RuntimeException), ex :
			print "Caught the exception", ex.message(), "->", ex.javaClass()
			print ex.stacktrace()
		except Exception, ex:
			print ex
			assert False
			
#	def testThrowException(self) :
#		d = {"throwIOException" : throwIOException, }
#		p = JProxy(self.jpype.exc.ExceptionThrower, dict=d)
#		
#		assert self.jpype.exc.ExceptionTest.delegateThrow(p)

	def testThrowException3(self) :
		d = {"throwIOException" : throwByJavaException, }
		p = JProxy(self.jpype.exc.ExceptionThrower, dict=d)
		
		assert self.jpype.exc.ExceptionTest.delegateThrow(p)

 	def testExceptionPYEXCName(self) :
 		e = self.jpype.exc.ChildTestException()
                name = "jpype.exc.ChildTestExceptionPyRaisable"
                assert name == e.PYEXC.__name__

 	def testExceptionInstanceof(self) :
 		e = self.jpype.exc.ChildTestException()
                assert isinstance(e, self.jpype.exc.ParentTestException)

 	def testExceptionPYEXCInstanceof(self) :
 		e = self.jpype.exc.ChildTestException()
                assert issubclass(e.PYEXC,
                                  self.jpype.exc.ParentTestException.PYEXC)

        def testThrowChildExceptionFromCatchJExceptionParentClass(self) :
		try :
			self.jpype.exc.ExceptionTest.throwChildTestException()
			assert False
		except JException(self.jpype.exc.ParentTestException), ex :
                        pyexc = self.jpype.exc.ChildTestException.PYEXC
                        assert isinstance(ex, pyexc)
