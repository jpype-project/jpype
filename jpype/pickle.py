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
JPype Pickle Module
--------------------

This module contains overloaded Pickler and Unpickler classes that operate
on Java classes. Pickling of Java objects is restricted to classes
that implement Serializable.  Mixed pickle files containing both
Java and Python objects are allowed.  Only one copy of each Java object
will appear in the pickle file even it is appears multiple times in the
data structure.

JPicklers and JUnpickler use Java ObjectOutputStream and ObjectInputStream
to serialize objects. All of the usual Java serialization errors may be
thrown.

This is backed by the native cPickler implementation.

Example:

.. code-block:: python

  myobj = jpype.JClass('java.util.ArrayList')()
  myobj.add("test")

  from jpype.pickle import JPickler, JUnpickler
  with open("test.pic", "wb") as fd:
    JPickler(fd).dump(myobj)

  with open("test.pic", "rb") as fd:
    newobj = JUnpickler(fd).load()


Proxies and other JPype specific module resources cannot be pickled currently.

Requires:
    Python 3.6 or later

"""
from __future__ import absolute_import
import _jpype
import pickle

from copyreg import dispatch_table


# TODO: Support use of a custom classloader with the unpickler.
# TODO: Use copyreg to pickle a JProxy

__ALL__ = ['JPickler', 'JUnpickler']

# This must exist as a global, the real unserializer is created by the JUnpickler


class JUnserializer(object):
    def __call__(self, *args):
        raise pickle.UnpicklingError("Unpickling Java requires JUnpickler")


class _JDispatch(object):
    """Dispatch for Java classes and objects.

    Python does not have a good way to register a reducer that applies to
    many classes, thus we will substitute the usual dictionary with a
    class that can produce reducers as needed.
    """

    def __init__(self, dispatch):
        self._encoder = _jpype.JClass('org.jpype.pickle.Encoder')()
        self._builder = JUnserializer()
        self._dispatch = dispatch

        # Extension dispatch table holds reduce method
        self._call = self.reduce

    # Python2 and Python3 _Pickler use get()

    def get(self, cls):
        if not issubclass(cls, (_jpype.JClass, _jpype.JObject)):
            return self._dispatch.get(cls)
        return self._call

    # Python3 cPickler uses __getitem__()
    def __getitem__(self, cls):
        if not issubclass(cls, (_jpype.JClass, _jpype.JObject)):
            return self._dispatch[cls]
        return self._call

    # For Python3
    def reduce(self, obj):
        byte = bytes(self._encoder.pack(obj))
        return (self._builder, (byte, ))


class JPickler(pickle.Pickler):
    """Pickler overloaded to support Java objects

    Parameters:
        file: a file or other writeable object.
        *args: any arguments support by the native pickler.

    Raises:
        java.io.NotSerializableException: if a class is not serializable or
            one of its members
        java.io.InvalidClassException: an error occures in constructing a
            serialization.

    """

    def __init__(self, file, *args, **kwargs):
        pickle.Pickler.__init__(self, file, *args, **kwargs)

        # In Python3 we need to hook into the dispatch table for extensions
        self.dispatch_table = _JDispatch(dispatch_table)


class JUnpickler(pickle.Unpickler):
    """Unpickler overloaded to support Java objects

    Parameters:
        file: a file or other readable object.
        *args: any arguments support by the native unpickler.

    Raises:
        java.lang.ClassNotFoundException: if a serialized class is not
            found by the current classloader.
        java.io.InvalidClassException: if the serialVersionUID for the
            class does not match, usually as a result of a new jar
            version.
        java.io.StreamCorruptedException: if the pickle file has been
            altered or corrupted.

    """

    def __init__(self, file, *args, **kwargs):
        self._decoder = _jpype.JClass('org.jpype.pickle.Decoder')()
        pickle.Unpickler.__init__(self, file, *args, **kwargs)

    def find_class(self, module, cls):
        """Specialization for Java classes.

        We just need to substitute the stub class for a real
        one which points to our decoder instance.
        """
        if cls == "JUnserializer":
            decoder = self._decoder

            class JUnserializer(object):
                def __call__(self, *args):
                    return decoder.unpack(args[0])
            return JUnserializer
        return pickle.Unpickler.find_class(self, module, cls)
