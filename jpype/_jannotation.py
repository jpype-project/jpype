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
from jpype import JClass, JString


__all__ = ['JAnnotation', 'JParameterAnnotation']


def _get_retention_policy(cls: JClass):
    retention = cls.class_.getAnnotation(JClass("java.lang.annotation.Retention"))
    if retention:
        return retention.value()


class _JAnnotationBase:

    __slots__ = ('_obj',)

    def __init__(self):
        self._obj = None

    def __call__(self, obj):
        self._obj = obj
        annotations = getattr(obj, "__jannotations__", None)
        if annotations is None:
            annotations = []
            setattr(obj, "__jannotations__", annotations)
        annotations.append(self)
        return obj


class JAnnotation(_JAnnotationBase):

    __slots__ = ('_decl', '_default')

    def __new__(cls, jcls, *args, **kwargs):
        """Object for all Java annotation instances.

        To specify annotations on a field, use a tuple. Use the "default" kwarg
        on the first annotation to specify the constand field value.
        In the context of a Java annotation, default is a keyword and therefore it
        cannot cause a conflict with an annotation's element.

        Returns:
          JAnnotation: a new wrapper for a Java annotation

        Raises:
          TypeError: if the component class is not an annotation.
          TypeError: if the annotation class does not have RUNTIME retention.
          ValueError: if JAnnotation is called directly
        """
        if not jcls.class_.isAnnotation():
            raise TypeError("%s is not an annotation" % jcls)
        RetentionPolicy = JClass("java.lang.annotation.RetentionPolicy")
        policy = _get_retention_policy(jcls)
        if policy != RetentionPolicy.RUNTIME:
            # I don't think class file retention is necessary because
            # something would have to read the class file data to know
            # if the annotation is present. There is no reason we can't
            # support it, I just don't see a reason to.
            raise TypeError("%s does not have runtime retention" % jcls)
        return super().__new__(JAnnotation)

    def __init__(self, cls, *args, **kwargs):
        super().__init__()
        AnnotationDecl = JClass("org.jpype.extension.AnnotationDecl")
        self._decl = AnnotationDecl(cls)
        IllegalArgumentException = JClass("java.lang.IllegalArgumentException")
        UnsupportedOperationException = JClass("java.lang.UnsupportedOperationException")
        self._default = kwargs.pop("default", None)
        if args:
            if callable(args[0]):
                # marker annotation on a function
                # no further checking required
                self._obj = args[0]
                return
            elements = {JString("value") : args[0]}
        else:
            elements = {JString(k) : v for k, v in kwargs.items()}
        try:
            self._decl.addElements(elements)
        except UnsupportedOperationException as e:
            fmt_args = (cls.class_.getSimpleName(), e.getMessage())
            raise KeyError("%s has no element '%s'", fmt_args)
        except IllegalArgumentException as e:
            fmt_args = (cls.class_.getSimpleName(), e.getMessage())
            raise KeyError("%s is missing a default value for the element '%s'" % fmt_args)


class JParameterAnnotation(_JAnnotationBase):

    __slots__ = ('_name', '_annotations')

    def __init__(self, name: str, *annotations):
        super().__init__()
        self._name = name
        self._annotations = annotations
        if not name:
            raise ValueError("The name of the parameter to annotate must be provided")
        if not annotations:
            raise ValueError("No annotations provided")


_jpype.JAnnotation = JAnnotation
