#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************

import _jpype
import _jclass

def _initialize() :
    _jpype.setWrapperClass(_JWrapper)
    _jpype.setStringWrapperClass(JString)

class _JWrapper(object) :
    def __init__(self, v) :
        if v is not None :
            self._value = _jpype.convertToJValue(self.typeName, v)
        else:
            self._value = None
            
    
class JByte(_JWrapper) :
    typeName = "byte"
    
class JInt(_JWrapper) :
    typeName = "int"

class JLong(_JWrapper) :
    typeName = "long"

class JFloat(_JWrapper) :
    typeName = "float"

class JDouble(_JWrapper) :
    typeName = "double"

class JChar(_JWrapper) :
    typeName = "char"

class JBoolean(_JWrapper) :
    typeName = "boolean"

class JString(_JWrapper) :
    typeName = "java.lang.String"
    
def _getDefaultTypeName(obj) :
    if obj is True or obj is False :
        return 'java.lang.Boolean'
        
    if isinstance(obj, str) or isinstance(obj, unicode) :
        return "java.lang.String"

    if isinstance(obj, int) :
        return "java.lang.Integer"
        
    if isinstance(obj, long) :
        return "java.lang.Long"
        
    if isinstance(obj, float) :
        return "java.lang.Double"

    if isinstance(obj, _jclass._JavaClass) :
        return obj.__javaclassname__
        
    if isinstance(obj, _jclass.java.lang.Class) :
        return obj.__class__.__javaclass__.getName()

    if instance(obj, _JWrapper) :
        return obj.typeName


    raise RuntimeException, "Unable to determine the default type of "+obj.__class__

class JObject(_JWrapper) :
    def __init__(self, v, tp = None) :
        if tp is None :
            tp = _getDefaultTypeName(v)
        if isinstance(tp, _jclass._JavaClass) :
            tp = tp.__javaclass__.getName()
            
        self.typeName = tp
        self._value = _jpype.convertToJValue(tp, v)