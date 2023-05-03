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

__all__ = ['JObject']


class JObject(_jpype._JObject, internal=True):  # type: ignore[call-arg]
    """ Base class for all object instances.

    It can be used to test if an object is a Java object instance with
    ``isinstance(obj, JObject)``.

    Calling ``JObject`` as a function can be used to covert or cast to
    specific Java type.  It will box primitive types and supports an
    option type to box to.

    This wrapper functions in three ways.

      - If the no type is given the object is automatically
        cast to type best matched given the value.  This can be used
        to create a boxed primitive.  ``JObject(JInt(i))``

      - If the type is a primitve, the object will be the boxed type of that
        primitive.  ``JObject(1, JInt)``

      - If the type is a Java class and the value is a Java object, the
        object will be cast to the Java class and will be an exact match to
        the class for the purposes of matching arguments. If the object
        is not compatible, an exception will be raised.

    Args:
       value: The value to be cast into an Java object.
       type(Optional, type): The type to cast into.

    Raises:
       TypeError: If the object cannot be cast to the specified type, or
         the requested type is not a Java class or primitive.

    """
    def __new__(cls, *args, **kwargs):
        if len(args) == 0:
            return _jpype._java_lang_Object()
        return _JObjectFactory(*args, **kwargs)


def _getDefaultJavaObject(obj):
    """ Determine the type of the object based the type of a value.

        Python primitives - lookup the type in the table
        Java primitive - lookup boxed type in the table
        Java objects - just use their type directly

    """
    tp = type(obj)
    # handle Python types and Java primitives
    try:
        return _jpype._object_classes[tp]
    except KeyError:
        pass

    # handle Class wrappers
    if isinstance(tp, _jpype._JClass):
        return tp

    # handle JProxy instances
    try:
        return obj.__javaclass__
    except AttributeError:
        pass

    raise TypeError(
        "Unable to determine the default type of `{0}`".format(tp.__name__))


def _JObjectFactory(v=None, tp=None):
    """ Creates a Java object.

    If not specified type is determined based on the object.
    If type type is specified then then it tried to box it.
    """
    if tp is None:
        # Automatically determine based on the value
        tp = _getDefaultJavaObject(v)
    elif isinstance(tp, str):
        tp = _jpype.JClass(tp)
    if tp in _jpype._object_classes:
        if not isinstance(tp, _jpype.JClass):
            import warnings
            warnings.warn("Using JObject with a Python type is deprecated.",
                          category=DeprecationWarning, stacklevel=3)
        tp = _jpype._object_classes[tp]

    # Given a Java class
    if isinstance(tp, _jpype._JClass):
        return tp._cast(v)

    raise TypeError("Invalid type conversion to %s requested." % tp)


# Hook up module resources
_jpype.JObject = JObject
