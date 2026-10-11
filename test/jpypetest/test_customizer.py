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
import jpype
from jpype.types import *
from jpype import java
import common
try:
    import numpy as np
except ImportError:
    pass


class CustomizerTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass('jpype.common.Fixture')()

    def testSticky(self):
        @jpype.JImplementationFor("jpype.override.A")
        class _A:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                pass

        A = jpype.JClass("jpype.override.A")
        B = jpype.JClass("jpype.override.B")
        self.assertEqual(A.remove, _A.remove)
        self.assertEqual(B.remove, _A.remove)
        self.assertEqual(str(A.remove_), "jpype.override.A.remove")
        self.assertEqual(str(B.remove_), "jpype.override.B.remove")

    def testStickyInheritanceAndInterfaceDepth(self):
        # General correctness coverage for sticky methods across an
        # inheritance chain and an interface extended farther down than
        # the customizer's target (I1 extends I0): every class - whether
        # it redeclares remove() itself or only inherits it - gets its
        # own correctly-captured original, and calling remove() reaches
        # the right Java implementation either way.
        #
        # This also covers https://github.com/jpype-project/jpype/issues/1473:
        # two independent sticky customizers registered for the same
        # target before any class in the hierarchy is created (e.g.
        # jpype's own built-in _JList for java.util.List, plus a second,
        # user-supplied List customizer - the actual shape of the
        # original report) get merged into one sticky list and applied
        # together in a single pass. The second customizer's capture must
        # not treat the first customizer's wrapper as "the real
        # original": that made the wrapper call itself once installed,
        # recursing without bound. Both customizers have to be registered
        # up front, in the same test, before IBase (or anything derived
        # from it) is created - once a class exists, a later registration
        # is not retroactively re-applied to it.
        @jpype.JImplementationFor("jpype.override.Overrides.I0")
        class _I0a:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                return self.remove_(obj) + 100

        @jpype.JImplementationFor("jpype.override.Overrides.I0")
        class _I0b:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                return self.remove_(obj) + 1000

        # A *different* target (I1, farther down than I0) with its own
        # sticky customizer, using the same rename as I0's - the "diamond"
        # case: ISubIface below reaches I0 both directly (extends IBase)
        # and through I1 (implements I1), so it ends up processed by both
        # I0's merged sticky pass and I1's separate one.
        @jpype.JImplementationFor("jpype.override.Overrides.I1")
        class _I1x:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                return self.remove_(obj) + 100000

        IBase = jpype.JClass("jpype.override.Overrides.IBase")
        ISub = jpype.JClass("jpype.override.Overrides.ISub")
        ISubIface = jpype.JClass("jpype.override.Overrides.ISubIface")
        ISubOverride = jpype.JClass("jpype.override.Overrides.ISubOverride")

        # Last customizer applied wins for the visible method (consistent
        # with JImplementationFor's documented conflict resolution for
        # ordinary members) - but remove_ must still be the true Java
        # original (1), not _I0a's wrapper. Before the fix, remove_ ended
        # up aliased to _I0a's wrapper and this recursed instead of
        # returning 1001.
        self.assertEqual(IBase().remove(None), 1001)
        self.assertEqual(str(IBase.remove_), "jpype.override.Overrides.IBase.remove")
        # Inherits IBase's real implementation - no override anywhere.
        # Does not implement I1, so unaffected by _I1x.
        self.assertEqual(ISub().remove(None), 1001)
        self.assertEqual(str(ISub.remove_), "jpype.override.Overrides.ISub.remove")
        # Inherits IBase's real implementation while also implementing a
        # sub-interface (I1) of the customizer's target (I0) - both I0's
        # pass and I1's pass apply, and the true original still survives
        # both without recursing: 1 (true original) + 100000 (I1's, the
        # last-applied wrapper in MRO order).
        self.assertEqual(ISubIface().remove(None), 100001)
        self.assertEqual(str(ISubIface.remove_), "jpype.override.Overrides.ISubIface.remove")
        # Redeclares remove() (own original 2) despite also implementing
        # I1 - still gets its own fresh rename, surviving both passes:
        # 2 + 100000.
        self.assertEqual(ISubOverride().remove(None), 100002)
        self.assertEqual(str(ISubOverride.remove_), "jpype.override.Overrides.ISubOverride.remove")

    def testStickyMismatchedRenameStack(self):
        # Three sticky customizers stacked on the same target, the last
        # using a *different* rename than the first two. The first two
        # compose safely (same shape as
        # testStickyInheritanceAndInterfaceDepth's double-customizer
        # case). The third cannot safely recover the true original under
        # its own rename - cls's own dict no longer holds an unwrapped
        # _JMethod by the time it runs, and the fix intentionally leaves
        # a mismatched rename target unset rather than guessing (guessing
        # wrong reintroduces #1473's recursion for the common case, same
        # rename reused across customizers). The result is a clean
        # AttributeError instead of a hang or wrong answer; no in-tree
        # customizer stacks mismatched renames today.
        @jpype.JImplementationFor("jpype.override.Overrides.IStack")
        class _StackA:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                return self.remove_(obj) + 100

        @jpype.JImplementationFor("jpype.override.Overrides.IStack")
        class _StackB:
            @jpype.JOverride(sticky=True, rename="remove_")
            def remove(self, obj):
                return self.remove_(obj) + 1000

        @jpype.JImplementationFor("jpype.override.Overrides.IStack")
        class _StackC:
            @jpype.JOverride(sticky=True, rename="orig_remove")
            def remove(self, obj):
                return self.orig_remove(obj) + 10000

        IStackImpl = jpype.JClass("jpype.override.Overrides.IStackImpl")

        # remove_ (StackA/StackB's rename) still correctly holds the true
        # original, untouched by StackC's mismatched rename attempt.
        self.assertEqual(str(IStackImpl.remove_), "jpype.override.Overrides.IStackImpl.remove")
        self.assertIsNone(IStackImpl.__dict__.get("orig_remove"))
        with self.assertRaises(AttributeError):
            IStackImpl().remove(None)

    def testRetroactiveCustomizerComposition(self):
        # https://github.com/jpype-project/jpype/issues/1476
        #
        # A second customizer for a target jpype has already instantiated
        # registers through a different path (_applyCustomizerPost,
        # "retroactive" registration) than a customizer registered before
        # any class exists. That path used to overwrite the target's
        # __jclass_init__ outright instead of chaining it with whatever
        # was already installed there (by an earlier registration, up
        # front or itself retroactive) - silently dropping the earlier
        # registration's sticky methods *and* explicit __jclass_init__
        # hooks, but only for classes created *after* the retroactive
        # registration (classes that already existed kept working, which
        # is what made this easy to miss).
        hook_calls = []

        @jpype.JImplementationFor("jpype.override.Overrides.IRetro")
        class _RetroA:
            def __jclass_init__(cls):
                hook_calls.append(('A', cls.__name__))

            @jpype.JOverride(sticky=True, rename="removeA_")
            def remove(self, obj):
                return self.removeA_(obj) + 100

        # Forces the retroactive path for the next registration: IRetro
        # (and IRetroImpl, its only implementer so far) already exist by
        # the time _RetroB below registers.
        IRetroImpl = jpype.JClass("jpype.override.Overrides.IRetroImpl")
        self.assertEqual(IRetroImpl().remove(None), 101)

        hook_calls.clear()

        @jpype.JImplementationFor("jpype.override.Overrides.IRetro")
        class _RetroB:
            def __jclass_init__(cls):
                hook_calls.append(('B', cls.__name__))

            @jpype.JOverride(sticky=True, rename="removeB_")
            def remove(self, obj):
                return self.removeB_(obj) + 1000

        hook_calls.clear()
        # Created only now, after _RetroB's retroactive registration -
        # the case that lost _RetroA's contribution entirely before the
        # fix.
        IRetroSub = jpype.JClass("jpype.override.Overrides.IRetroSub")

        # Both hooks fired for IRetroSub specifically - neither the
        # up-front (_RetroA) nor the retroactive (_RetroB) registration
        # was silently dropped.
        self.assertIn(('A', 'jpype.override.Overrides.IRetroSub'), hook_calls)
        self.assertIn(('B', 'jpype.override.Overrides.IRetroSub'), hook_calls)
        # _RetroA's sticky rename survives for IRetroSub too - before the
        # fix this was never set at all for a class created after the
        # retroactive registration.
        self.assertEqual(str(IRetroSub.removeA_), "jpype.override.Overrides.IRetroSub.remove")
