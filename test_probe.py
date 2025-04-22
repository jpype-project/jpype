import jpype
import _jpype
import numpy as np

jpype.startJVM()

_jpype._bridge_concrete[object] = "PyType"

_jpype._bridge_concrete[bytearray] = "PyType"
_jpype._bridge_concrete[bytes] = "PyType"
_jpype._bridge_concrete[complex] = "PyType"
_jpype._bridge_concrete[dict] = "PyType"
_jpype._bridge_concrete[enumerate] = "PyType"
_jpype._bridge_concrete[BaseException] = "PyType"
_jpype._bridge_concrete[list] = "PyType"
_jpype._bridge_concrete[memoryview] = "PyType"
_jpype._bridge_concrete[range] = "PyType"
_jpype._bridge_concrete[set] = "PyType"
_jpype._bridge_concrete[slice] = "PyType"
_jpype._bridge_concrete[str] = "PyType"
_jpype._bridge_concrete[tuple] = "PyType"
_jpype._bridge_concrete[type] = "PyType"
_jpype._bridge_concrete[zip] = "PyType"

_jpype.probe(object)
_jpype.probe(slice)
_jpype.probe(list)
_jpype.probe(tuple)
_jpype.probe({})
_jpype.probe(iter([]))
_jpype.probe(enumerate)
_jpype.probe(np.array([]))

