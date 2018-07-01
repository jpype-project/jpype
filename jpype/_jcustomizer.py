#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
import _jpype

_JP_BASES = {}
_JP_CUSTOMIZERS = []

__all__ = ['registerClassBase','registerClassCustomizer']

def registerClassBase(name, cls):
    if name in _JP_BASES:
        _JP_BASES[name].append(cls)
    else:
        _JP_BASES[name]=[cls]

def registerClassCustomizer(c):
    _JP_CUSTOMIZERS.append(c)

# FIXME we should not just have a giant list of customizers that we apply every time.
# Most customizers apply either to a specific class, all classes that derive from,
# or all classes.  The customizer should defined what type it applies to so that 
# we can directly look up if it is applicable, rather than searching all.  However,
# this would be a heavy refactor as every customizer would need to be refactored.

# FIXME customizers currently only apply to classes after the customizer is loaded.
# this creates issues for bootstrapping especially with classes like basic arrays
# which may be overloaded to support operations like __add__.  

# FIXME this file was split from _jclass as we are now refactoring such that
# array classes and object classes are treated as the same.  
# Also when we refactor to remove the need for a specialized meta class the customizers
# will all likely change as both static and instance will appear on the same 
# list.  For now it is pretty lonely, but it will grow.
    

class JClassCustomizer(object):
    def canCustomize(self, name, jc):
        """ Determine if this class can be customized by this customizer.

        Classes should be customized on the basis of an exact match to 
        the class name or if the java class is derived from some base class.

        Args:
          name (str): is the name of the java class being created.
          jc (_jpype.PyJPClass): is the java class wrapper. 

        Returns:
          bool: true if customize should be called, false otherwise.
        """
        pass

    def customize(self, name, jc, bases, members):
        """ Customize the class.  

        Should be able to handle keyword arguments to support changes in customizers.

        Args:
          name (str): is the name of the java class being created.

        """
        pass

def _applyCustomizers(name, jc, bases, members):
    """ Called by JClass and JArray to customize a newly created class."""
    for i in _JP_CUSTOMIZERS:
        if i.canCustomize(name, jc):
            i.customize(name, jc, bases, members)
    if name in _JP_BASES:
        bases.extend(_JP_BASES[name])


