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
import jpype
from jpype import JString, java, JArray
import unittest
import common
import time

def suite() :
	return unittest.makeSuite(AttributeTestCase)

class AttributeTestCase(common.JPypeTestCase) :
	def setUp(self) :
		common.JPypeTestCase.setUp(self)
		self.__jp = self.jpype.attr
		
	def testWithBufferStrategy(self):
		j = jpype.JPackage("jpype").attr.ClassWithBuffer
		print j
		
	def testCallOverloadedMethodWithCovariance(self):
		# This is a JDk5-specific problem.
		
		h = jpype.java.lang.StringBuffer()
		h.delete(0, 0)
		
 	def testCallStaticString(self) :
 		h = self.__jp.Test1()	
 		v = h.testString(JString("abcd"), JString("abcde"))
 		
 		assert v[0] == 'abcd'
 		assert v[1] == 'abcde'
 		
 	def testCallStaticUnicodeString(self) :
 		h = self.__jp.Test1()	
 		v = h.testString(JString(u"abcd"), JString(u"abcde"))
 		
 		assert v[0] == 'abcd'
 		assert v[1] == 'abcde'
 
 	def testCallString(self) :
 		v = self.__jp.Test1.testStaticString("a", "b")
 		assert v[0] == 'a'
 		assert v[1] == 'b'
 
 	def testCallUnicodeString(self) :
 		v = self.__jp.Test1.testStaticString(u"a", u"b")
 		assert v[0] == 'a'
 		assert v[1] == 'b'
 
 	def testCallStringWithNone(self) :
 		v = self.__jp.Test1.testStaticString("a", None)
 		assert v[0] == 'a'
 		assert v[1] == None
 
 	def testWithHolder(self) :
 		print 'testWithHolder'
 		holder = self.__jp.Holder()
 		holder.f = "ffff"
 		assert holder.f == "ffff"
 		self.__jp.Test1.testStaticHolder(holder)
 	
 	def testWithSubHolder(self) :
 		h2 = self.__jp.SubHolder()
 		h2.f = "subholder"
 		self.__jp.Test1.testStaticHolder(h2)
 	
 	def testCallWithArray(self) :
 		h2 = self.__jp.Test1()
 		StringArray = JArray(JString)
 		v = StringArray(["Foo", "bar"])
 		t = self.__jp.Test1()
 		t.testStringArray(v)
 
 	def testGetStaticValue(self) :
 		assert str(self.__jp.Test1.objectValue) == "234"
 		
 	def testGetStaticByInstance(self) :
 		h = self.__jp.Test1()
 		assert str(h.objectValue) == "234"
 		
 	def testGetNonStatic(self) :
 		h = self.__jp.Test1()
 		assert h.stringValue == "Foo"
 
 	def testSetStaticValue(self) :
 		self.__jp.Test1.objectValue = java.lang.Integer(43)
 		assert str(self.__jp.Test1.objectValue) == "43"
 		self.__jp.Test1.reset()
 
 	def testSetNonStaticValue(self) :
 		h = self.__jp.Test1()
 		h.stringValue="bar"
 		assert h.stringValue == "bar"
 
 	def testReturnSubClass(self) :
 		h = self.__jp.Test1()
 		v = h.getSubClass()
 		assert isinstance(v, self.__jp.SubHolder)
 		
 	def testCallWithClass(self) :
 		h = self.__jp.Test1()
 		h.callWithClass(java.lang.Comparable)
 		
 	def testCallSuperclassMethod(self) :
 		h = self.__jp.Test2()
 		h.test2Method()
 		h.test1Method()		
 		
 	def testCallWithLong(self) :
 		h = self.__jp.Test1()
 		l = long(123)
 		
 		h.setByte(l)
 		h.setShort(l)
 		h.setInt(l)
 		
 	def testCharAttribute(self) :
 		h = self.__jp.Test1()
 		h.charValue = u'b'
 		
 		print h.charValue
 		print repr(h.charValue)
 		
 		assert h.charValue == u'b'
 		
 	def testGetPrimitiveType(self) :
 		Integer = jpype.JClass("java.lang.Integer")
 		intType = Integer.TYPE
 		
 	def testDifferentiateClassAndObject(self) :
 		h = self.__jp.Test1()
 		
 		assert h.callWithSomething(self.__jp.Test1) == u"Class"
 		assert h.callWithSomething(jpype.JObject(self.__jp.Test1, jpype.java.lang.Object)) == u"Object"

 	def testToString(self):
 		h = self.__jp.Test1()
 		assert str(h) == 'aaa'
 
 	def testSuperToString(self):
 		h = self.__jp.Test2()
 		print h
 		assert str(h) == 'aaa'
 		del h
		
#	def testStringToConversion(self):
#		try :
#			jpype.ConversionConfig.string = False
#			for i in range(1) :
#				h = self.__jp.Test1()
#				
#				start = time.time();
#				for j in range(10):
#					ts = h.getBigString()
#				stop = time.time()
#				print ts.__class__, (stop-start), 'ms'
#				
#				# for comparison ... convert a string to JStrng and back
#				s = "".join([" "]*1024*1024*5)
#				start = time.time()
#				js = JString(s)
#				stop = time.time()
#				print "converted to JString in", (stop-start), 'ms', len(s)
#				
#				start = time.time()
#				cs = str(JString(s))
#				print cs
#				print "converted to String in", (stop-start), 'ms', len(cs), cs
#		finally :
#			jpype.ConversionConfig.string = True
		
		
		