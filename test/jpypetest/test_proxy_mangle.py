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
import jpype
import _jpype
from jpype import _jbridge
import common


_OBJECT_METHOD_NAMES = frozenset(("equals", "hashCode", "toString"))


def _mangle(name):
    # Mirrors org.jpype.proxy.ProxyType.mangle(String) exactly - see
    # plan/NameMangling.md. Any drift between the two must fail this test.
    # equals/hashCode/toString are never mangled regardless of `mangle`:
    # java.lang.reflect.Proxy special-cases these three and always
    # dispatches via Object.class's own Method (ProxyFactory.objectMethods),
    # never through ProxyType.buildDescriptor's interface-specific
    # descriptor - see ProxyType.isObjectMethodSignature. Confirmed
    # empirically: PyObjectNGTest.testIdentity/PyExcNGTest regressed when
    # these were mangled, because the real runtime dispatch never looks at
    # the mangled key.
    if name in _OBJECT_METHOD_NAMES:
        return name
    if name.startswith("$"):
        return name[1:]
    return "." + name


class ProxyMangleCompletenessTestCase(common.JPypeTestCase):
    """Guards plan/NameMangling.md's atomic Java-trigger/Python-key-rename
    pair: for every interface that is mangle-eligible (descends from
    python.lang.PyObject) and registered as a proxy dispatch table, every
    abstract method it declares must have a correspondingly-mangled key in
    that table. A missed rename here is not a loud failure at runtime - see
    ProxyInstance.hostInvoke's silent no-op on a missing callable - so this
    is the regression guard that turns it into an immediate test failure.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        # Force every still-lazy SPI class to resolve now, the same way
        # _LazyCache does on first real probe, so this test covers the
        # lazily-registered interfaces (io.Buffered*) too, not just the
        # eagerly-registered ones already sitting in _jpype._methods.
        pending = _jpype._lazy_pending
        for modname in list(pending.keys()):
            for clsname, (javaInterface, methodsSource) in pending.pop(modname).items():
                _jbridge._installer_register_class(modname, clsname, javaInterface, methodsSource)

    def testAllMangleEligibleInterfacesHaveCompleteKeys(self):
        PyObject = jpype.JClass("python.lang.PyObject")
        Bypass = jpype.JClass("org.jpype.annotation.Bypass")
        Builtin = jpype.JClass("org.jpype.annotation.Builtin")
        Modifier = jpype.JClass("java.lang.reflect.Modifier")

        # At runtime a proxy for `iface` isn't dispatched off iface's own
        # dict alone - pyjp_probe.cpp's finalizeMethods walks the full
        # interface closure (iface plus every ancestor interface reachable
        # via extends) and PyDict_Update-merges each ancestor's own
        # registered dict together (e.g. PyObject's hashCode/equals/toString
        # entries cover every descendant). So a method is only really
        # "missing" if no interface anywhere in that closure supplies its
        # mangled key - replicate that merge here before checking, keyed off
        # of Java's own isAssignableFrom rather than reimplementing the
        # C++ crawl.
        all_entries = list(_jpype._methods.items())

        checked = 0
        for iface, _ in all_entries:
            if not PyObject.class_.isAssignableFrom(iface.class_):
                continue
            checked += 1
            merged = {}
            for other_iface, other_methods in all_entries:
                if other_iface.class_.isAssignableFrom(iface.class_):
                    merged.update(other_methods)

            # getMethods() (not getDeclaredMethods) returns one Method per
            # (name, declaring interface) pair, so a signature can show up
            # more than once - e.g. java.util.List's own plain abstract
            # `add(Object)` alongside PySequence's `default` override of the
            # same signature. ProxyType.buildDescriptor resolves this via
            # bestBySignature/isBetter (native/.../ProxyType.java): only
            # abstract if NO candidate anywhere for that signature is
            # default/@Bypass. Mirror that grouping here rather than
            # requiring a callable for every raw Method entry.
            by_signature = {}
            for method in iface.class_.getMethods():
                if Modifier.isStatic(method.getModifiers()):
                    continue
                params = tuple(str(p.getName()) for p in method.getParameterTypes())
                key = (str(method.getName()), params)
                by_signature.setdefault(key, []).append(method)

            missing = []
            for (name, _params), candidates in by_signature.items():
                if any(c.isDefault() or c.isAnnotationPresent(Bypass) for c in candidates):
                    continue
                if any(c.isAnnotationPresent(Builtin) for c in candidates):
                    # ProxyType.buildDescriptor special-cases @Builtin as a
                    # native MethodHandle backdoor (bypass=true at the
                    # descriptor level even though the method itself is
                    # abstract with no @Bypass annotation) - never routed to
                    # a Python callable, see ProxyType.java.
                    continue
                # Only python.lang/python.io interfaces are mangle-scoped by
                # plan/NameMangling.md; a plain java.util.List/Collection
                # abstract method (e.g. add/clear/indexOf/toArray on
                # PySequence's List<T> superinterface) was never routed
                # through the Python dispatch map before this change either
                # - that's pre-existing unimplemented API surface, not
                # something the mangling rename could have broken.
                if not all(str(c.getDeclaringClass().getPackageName()).startswith("python.")
                           for c in candidates):
                    continue
                wire = _mangle(name)
                if wire not in merged:
                    missing.append(wire)
            self.assertEqual(
                missing, [],
                "%s is missing mangled keys for abstract methods (checked "
                "across its full registered interface closure): %s"
                % (iface, missing))
        # Sanity: make sure the scan actually walked a nontrivial number of
        # PyObject-descended interfaces rather than silently checking zero
        # (e.g. because _jpype._methods keys stopped being JClass objects).
        self.assertGreater(checked, 10)
