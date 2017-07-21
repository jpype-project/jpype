#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
import _jpype
from . import _jclass
from ._pykeywords import KEYWORDS

_PROPERTY_ACCESSOR_PREFIX_LEN = 3

def _initialize() :
    _jclass.registerClassCustomizer(PropertiesCustomizer())

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
        if not (len(name) > _PROPERTY_ACCESSOR_PREFIX_LEN \
                        and _is_java_method(member)):
            continue
        access, rest = ( name[:_PROPERTY_ACCESSOR_PREFIX_LEN],
                         name[_PROPERTY_ACCESSOR_PREFIX_LEN:] )
        property_name = rest[:1].lower() + rest[1:]
        if property_name in members:
            if _is_java_method(members[property_name]):
                continue
        if access == 'get' and member.isBeanAccessor():
            try:
                pair = accessor_pairs[property_name]
                pair[0] = member
            except KeyError:
                accessor_pairs[property_name] = [member, None]
        elif access == 'set' and member.isBeanMutator():
            try:
                pair = accessor_pairs[property_name]
                pair[1] = member
            except KeyError:
                accessor_pairs[property_name] = [None, member]
    return accessor_pairs

def _is_java_method(attribute):
    return isinstance(attribute, _jpype._JavaMethod)

class PropertiesCustomizer(object) :
    def canCustomize(self, name, jc) :
        return True

    def customize(self, class_name, jc, bases, members) :
        accessor_pairs = _extract_accessor_pairs(members)
        for attr_name, (getter, setter) in accessor_pairs.items():
            # class is will be static to match Type.class in Java
            if attr_name=='class':
                continue
            # Handle keyword conflicts
            if attr_name in KEYWORDS:
                attr_name += "_"
            if attr_name in members:
                if not getter:
                    # add default getter if we
                    # only have a setter
                    getter = members[attr_name].fget
                elif not setter:
                    # add default setter if we
                    # only have a getter
                    setter = members[attr_name].fset
            members[attr_name] = property(getter, setter)
