from . import _jclass
from . import _jobject
import pickle
from copyreg import dispatch_table

__ALL__=['JPickler','JUnpickler']

# This must exist as a global, the real unserializer is created by the JUnpickler
class JUnserializer(object):
    def __call__(self, *args):
        raise pickle.UnpicklingError("Unpickling Java requires JUnpickler")

class JClassDispatch(object):
    """Dispatch for Java classes and objects.

    Python does not have a good way to register a reducer that applies to 
    many classes, thus we will substitute the usual dictionary with a 
    class that can produce reducers as needed.
    """
    def __init__(self):
        self._encoder = _jclass.JClass("org.jpype.pickle.Encoder")()
        self._builder = JUnserializer()

    def get(self, type):
        return self.__getitem__(type)

    def __getitem__(self, cls):
        if not issubclass(cls, (_jclass.JClass, _jobject.JObject)):
            return dispatch_table[cls]
        return self.reduce

    def reduce(self, obj):
        byte = self._encoder.pack(obj)[:].tobytes()
        return (self._builder, (byte, ))

class JPickler(pickle.Pickler):
    """Pickler overloaded to support Java objects
    """
    def __init__(self, file, *args, **kwargs):
        self.dispatch_table = JClassDispatch()
        pickle.Pickler.__init__(self, file, *args, **kwargs)

class JUnpickler(pickle.Unpickler):
    """Unpickler overloaded to support Java objects
    """
    def __init__(self, file, *args, **kwargs):
        self._decoder = _jclass.JClass("org.jpype.pickle.Decoder")()
        pickle.Unpickler.__init__(self, file, *args, **kwargs)

    def find_class(self, module, cls):
        """Specialization for Java classes.  

        We just need to substitute the stub class for a real
        one which points to our decoder instance.
        """
        if cls=="JUnserializer":
            decoder = self._decoder
            class JUnserializer(object):
                def __call__(self, *args):
                    return decoder.unpack(args[0])
            return JUnserializer
        return pickle.Unpickler.find_class(self, module, cls)
