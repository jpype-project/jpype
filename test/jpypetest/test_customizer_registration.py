"""Rejected late base registration must not change the registry."""
import subprocess
import sys
import unittest


class CustomizerRegistrationTest(unittest.TestCase):
    def testRejectedLateBaseLeavesRegistrationUnchanged(self):
        script = r'''
import jpype
from jpype import _jcustomizer

jpype.startJVM()
try:
    name = "java.util.zip.CRC32"
    cls = jpype.JClass(name)
    hints = _jcustomizer.getClassHints(name)
    assert hints.instantiated
    before = list(hints.bases)
    class RejectedBase:
        pass
    try:
        jpype.JImplementationFor(name, base=True)(RejectedBase)
    except TypeError:
        pass
    else:
        raise AssertionError("late base registration was accepted")
    after = list(hints.bases)
    @jpype.JImplementationFor(name)
    class Marker:
        def registrationMarker(self):
            return "registered"
    assert cls().registrationMarker() == "registered"
    assert after == before, "rejected registration changed the bases registry"
finally:
    jpype.shutdownJVM()
'''
        result = subprocess.run([sys.executable, "-c", script],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
