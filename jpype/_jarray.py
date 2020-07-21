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
import _jpype
from . import _jcustomizer


__all__ = ['JArray']


class JArray(_jpype._JObject, internal=True):
    """ Creates a Java array class for a Java type of a given dimension.

    This serves as a base type and factory for all Java array classes.
    The resulting Java array class can be used to construct a new
    array with a given size or specified members.

    JPype arrays support Python operators for iterating, length, equals, 
    not equals, subscripting, and slicing. They also support Java
    object methods, clone, and length property. Java arrays may not
    be resized, and as such elements cannot be added nor deleted. Currently,
    applying the slice operator produces a new Python sequence.

    Example:
        .. code-block:: python

          # Define a new array class for ``int[]``
          IntArrayCls = JArray(JInt)

          # Create an array holding 10 elements
          #   equivalent to Java ``int[] x=new int[10]``
          x = IntArrayCls(10)

          # Create a length 3 array initialized with [1,2,3]
          #   equivalent to Java ``int[] x = new int[]{1,2,3};``
          x = IntArrayCls([1,2,3])

          # Operate on an array
          print(len(x))
          print(x[0])
          print(x[:-2])
          x[1:]=(5,6)

          if isinstance(x, JArray):
               print("object is a java array")

          if issubclass(IntArrayCls, JArray):
               print("class is a java array type.")

    Args:
      javaClass (str,type): Is the type of element to hold in
        the array.
      ndims (Optional,int): the number of dimensions of the array
        (default=1)

    Returns:
      A new Python class that representing a Java array class.

    Raises:
      TypeError: if the component class is invalid or could not be found.

    Note:
      javaClass can be specified in three ways:

        - as a string with the name of a java class.
        - as a Java primitive type such as ``jpype.JInt``.
        - as a Java class type such as ``java.lang.String``.


    """
    def __new__(cls, tp, dims=1):
        if cls != JArray:
            raise TypeError("Arrays factory can't be used as type")
        jc = _toJavaClass(tp)
        return _jpype._newArrayType(jc, dims)

    @classmethod
    def of(cls, array, dtype=None):
        return _jpype.arrayFromBuffer(array, dtype)


class _JArrayProto(object):

    def __str__(self):
        return str(list(self))

    def __iter__(self):
        return _JavaArrayIter(self)

    def __reversed__(self):
        for elem in self[::-1]:
            yield elem

    def clone(self):
        """ Clone the Java array.

        Create a "shallow" copy of a Java array. For a
        single dimensional array of primitives, the cloned array is
        complete independent copy of the original. For objects or
        multidimensional arrays, the new array is a copy which points
        to the same members as the original.

        To obtain a deep copy of a Java array, use Java serialize and
        deserialize operations to duplicate the entire array and
        contents. In order to deep copy, the objects must be
        Serializable.

        Returns:
            A shallow copy of the array.
        """
        return _jpype.JClass("java.util.Arrays").copyOf(self, len(self))


def _toJavaClass(tp):
    """(internal) Converts a class type in python into a internal java class.

    Used mainly to support JArray.

    The type argument will operate on:
     - (str) lookup by class name or fail if not found.
     - (JClass) just returns the java type.
     - (type) uses a lookup table to find the class.
    """
    # if it a string than we lookup the class by name.
    if isinstance(tp, str):
        return _jpype._java_lang_Class.forName(tp)

    # if is a java.lang.Class instance, then no coversion required
    if isinstance(tp, _jpype._JClass):
        return tp

    # Okay then it must be a type
    try:
        return _jpype._type_classes[tp].class_
    except KeyError:
        pass

    # See if it a class type
    try:
        return tp.class_
    except AttributeError:
        pass

    raise TypeError("Unable to find class for '%s'" % tp.__name__)


# FIXME are these not sequences?  They act like sequences but are they
# connected to collections.abc.Sequence
# has: __len__, __iter__, __getitem__
# missing: __contains__ (required for in)
# Cannot be Mutable because java arrays are fixed in length


class _JavaArrayIter(object):
    def __init__(self, a):
        self._array = a
        self._ndx = -1

    def __iter__(self):
        return self

    def __next__(self):
        self._ndx += 1
        if self._ndx >= len(self._array):
            raise StopIteration
        return self._array[self._ndx]

    next = __next__

# **********************************************************
# Char array customizer


@_jcustomizer.JImplementationFor("byte[]")
@_jcustomizer.JImplementationFor("char[]")
class _JCharArray(object):
    def __str__(self):
        return str(_jpype.JString(self))

    def __eq__(self, other):
        if isinstance(other, str):
            return str(self) == other
        try:
            return self.equals(self.__class__(other))
        except TypeError:
            return False
    __hash__ = _jpype._JObject.__hash__


# Install module hooks
_jcustomizer._applyCustomizerPost(_jpype._JArray, _JArrayProto)
_jpype.JArray = JArray
