# *****************************************************************************
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
#   See NOTICE file for details.
#
# *****************************************************************************
"""
JPype Beans Module
------------------

This customizer finds all occurences of methods with get or set and converts
them into Python properties. This behavior is sometimes useful in programming
with JPype with interactive shells, but also leads to a lot of confusion.
Is this class exposing a variable or is this a property added JPype.  

As an unnecessary behavior that violates both the Python principle
*"There should be one-- and preferably only one --obvious way to do it."* and
the C++ principle *"You only pay for what you use"*.  This misfeature
was removed from the distribution as a default. However, given that it is
useful to have methods appear as properties, it was moved to a
an optional module.

To use beans as properties:

.. code-block:: python

  import jpype.beans

The beans property modification is a global behavior and applies retroactively
to all classes currently loaded.  Once started it can not be undone.
"""
import _jpype
from . import _jcustomizer
from ._pykeywords import pysafe


def _extract_accessor_pairs(members):
    """Extract pairs of corresponding property access methods
    (getter and setter) from a Java class's members (attributes).

    If a public method with a property's name exists no pair for
    that property will be extracted.

    Returns a dictionary with the property name as key and a tuple
    of (getter method, setter method) as value. A tuple element
    value might be `None` if only a getter or only a setter
    exists.
    """
    accessor_pairs = {}

    for name, member in members.items():
        if not isinstance(member, _jpype._JMethod) or len(name) <= 3:
            continue
        if name == "getClass":
            continue

        if member._isBeanAccessor():
            property_name = name[3].lower() + name[4:]
            try:
                pair = accessor_pairs[property_name]
                pair[0] = member
            except KeyError:
                accessor_pairs[property_name] = [member, None]
        elif member._isBeanMutator():
            property_name = name[3].lower() + name[4:]
            try:
                pair = accessor_pairs[property_name]
                pair[1] = member
            except KeyError:
                accessor_pairs[property_name] = [None, member]
    return accessor_pairs


@_jcustomizer.JImplementationFor("java.lang.Object")
class _BeansCustomizer(object):
    """ Add properties for get/set Bean patterns found in classes.  """

    def __jclass_init__(self):
        accessor_pairs = _extract_accessor_pairs(self.__dict__)
        for attr_name, (getter, setter) in accessor_pairs.items():
            attr_name = pysafe(attr_name)

            # Don't mess with an existing member
            if attr_name in self.__dict__:
                continue

            # Add the property
            self._customize(attr_name, property(getter, setter))
