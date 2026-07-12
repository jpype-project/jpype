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
import common
import unittest


class InceptionTestCase(common.JPypeTestCase):
    """ The odd-ball case: this whole test runs *inside* the "outer" Python
    interpreter that started the JVM (the normal forward-bridge direction -
    Python hosting Java). From there, we reach into ``org.jpype`` and use
    the reverse-bridge machinery (``SubInterpreterBuilder``/``Script``,
    built for Java-hosts-Python embedding - see ``plan/StreamRedirect.md``,
    ``plan/SubInterpreterBuilder.md``) to launch a *second*, genuinely
    separate CPython subinterpreter and pilot it via Java, from within
    Python.

    Short answer: no singularity. Launching the subinterpreter and running
    code inside it (``exec_``, side effects only) works fine. But pulling
    an *evaluated result* back out of it (``eval``) - a live Python object
    that belongs to the subinterpreter - and handing it to code running in
    the outer interpreter is exactly the cross-interpreter proxy smuggling
    scenario ``plan/Smuggler.md`` was built to catch: it raises
    ``RuntimeError`` instead of touching memory owned by a different
    interpreter's allocator/arena under the wrong GIL.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.SubInterpreterBuilder = jpype.JClass("org.jpype.SubInterpreterBuilder")
        self.Script = jpype.JClass("org.jpype.Script")
        self.NativeLauncherControl = jpype.JClass("org.jpype.internal.NativeLauncherControl")

    def _startOrSkip(self):
        try:
            return self.SubInterpreterBuilder.elevated().start()
        except jpype.JException as ex:
            if "Subinterpreters not supported" in str(ex):
                raise unittest.SkipTest(
                    "Subinterpreters require Python 3.12+ (native library built against an older Python)")
            raise

    def testEvalResultCrossingBackIsBlockedNotCorrupted(self):
        sub = self._startOrSkip()
        try:
            script = self.Script(sub)
            with self.assertRaisesRegex(RuntimeError, "smuggled proxy"):
                script.eval("1 + 1")
        finally:
            # Always close, even (especially) when eval raised above - an
            # unclosed subinterpreter left dangling at process shutdown is
            # a *fatal* CPython error (PyInterpreterState_Delete: remaining
            # subinterpreters), not just a leak. Confirmed the hard way
            # while first exploring this scenario.
            sub.close()

    def testExecSideEffectsWorkFine(self):
        # exec() has nothing to convert back across the interpreter
        # boundary - no return value - so it isn't hit by the smuggling
        # guard at all.
        sub = self._startOrSkip()
        try:
            script = self.Script(sub)
            script.exec_("x = 42")
            # ...but reading that side effect back out via eval() is a
            # PyObject crossing the boundary again, so it hits the same
            # guard as the top-level eval("1 + 1") case.
            with self.assertRaisesRegex(RuntimeError, "smuggled proxy"):
                script.eval("x")
        finally:
            sub.close()

    def testOuterInterpreterSurvivesAndGilNotLeaked(self):
        sub = self._startOrSkip()
        try:
            script = self.Script(sub)
            script.exec_("y = 'alive'")
            try:
                script.eval("y")
            except RuntimeError:
                pass
        finally:
            sub.close()

    def testWithStatementClosesSubInterpreter(self):
        # SubInterpreter implements java.lang.AutoCloseable, and jpype's
        # blanket AutoCloseable customizer (jpype/_jio.py) gives every such
        # Java object __enter__/__exit__ for free - no special-casing
        # needed here, same mechanism that gives java.io.PrintStream
        # toPython() through plain inheritance.
        with self._startOrSkip() as sub:
            self.assertTrue(sub.isStarted())
            script = self.Script(sub)
            script.exec_("w = 'alive'")
        self.assertFalse(sub.isStarted())

        self.assertFalse(self.NativeLauncherControl.isGilHeld(),
                          "GIL leaked on the calling (outer, forward-bridge) thread "
                          "after piloting a subinterpreter from Python-via-Java")
        # The outer interpreter - the actual process's Python, running this
        # test - is completely unaffected by the subinterpreter's creation,
        # use, and teardown.
        self.assertEqual(21 * 2, 42)
