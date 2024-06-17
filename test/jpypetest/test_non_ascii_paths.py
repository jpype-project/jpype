"""
Regression test for https://github.com/jpype-project/jpype/issues/1194
"""

import unittest
import jpype
from pathlib import Path
import shutil
from multiprocessing import get_context, Queue, Process


class TestNonAsciiPaths(unittest.TestCase):

    def setUp(self):
        # Create a copy of the mrjar.jar file with a non-ASCII path.
        mrjar_path = Path("test/jar/mrjar.jar")
        self.mrjar_non_ascii = Path("test/jar/à_non_ascii/mrjar.jar")
        self.mrjar_non_ascii.parent.mkdir(exist_ok=True)
        shutil.copy(mrjar_path, self.mrjar_non_ascii)

    def tearDown(self):
        # Clean up the non-ASCII path.
        self.mrjar_non_ascii.unlink()
        self.mrjar_non_ascii.parent.rmdir()

    @staticmethod
    def _subproc_test_non_ascii_paths(path: str, queue: Queue):
        # Can `dir` be called on a package with a non-ASCII path?
        jpype.addClassPath(path)
        jpype.startJVM(jpype.getDefaultJVMPath())
        queue.put(dir(jpype.JPackage("org.jpype.mrjar")))
        jpype.shutdownJVM()

    def test_non_ascii_paths(self):
        # Uses a separate process to avoid polluting the main process with
        # a JVM instance, since restarting the JVM is not supported.
        ctx = get_context("spawn")
        queue: Queue = ctx.Queue()
        path = self.mrjar_non_ascii.absolute().as_posix()
        proc: Process = ctx.Process(
            target=self._subproc_test_non_ascii_paths, args=(path, queue)
        )
        proc.start()
        proc.join(5)
        assert queue.get() == ["A", "B", "sub"]
