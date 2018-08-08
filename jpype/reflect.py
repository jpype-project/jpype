# *****************************************************************************
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
# *****************************************************************************
import _jpype
from . import _core

# FIXME this class is entirely replaced with calls to .class_ and should likely be deprecated.


@_core.deprecated("class_.getConstructors()")
def getConstructors(clas):
    return clas.class_.getConstructors()[:]


@_core.deprecated("class_.getDeclaredConstructors()")
def getDeclaredConstructors(clas):
    return clas.class_.getDeclaredConstructors()[:]


@_core.deprecated("class_.getDeclaredFields()")
def getDeclaredFields(clas):
    '''Returns an array of Field objects reflecting all the fields declared by the class or interface represented by this Class object.'''
    return clas.class_.getDeclaredFields()[:]


@_core.deprecated("class_.getDeclaredMethods()")
def getDeclaredMethods(clas):
    '''Returns an array of Method objects reflecting all the methods declared by the class or interface represented by this Class object.'''
    return clas.class_.getDeclaredMethods()[:]


@_core.deprecated("class_.getFields()")
def getFields(clas):
    '''Returns an array containing Field objects reflecting all the accessible public fields of the class or interface represented by this Class object.'''
    return clas.class_.getFields()[:]


@_core.deprecated("class_.getMethods()")
def getMethods(clas):
    '''Returns an array containing Method objects reflecting all the public member methods of the class or interface represented by this Class object, including those declared by the class or interface and those inherited from superclasses and superinterfaces.'''
    return clas.class_.getMethods()[:]


@_core.deprecated("class_.getModifiers()")
def getModifiers(clas):
    '''Returns the Java language modifiers for this class or interface, encoded in an integer.'''
    return clas.class_.getModifiers()
