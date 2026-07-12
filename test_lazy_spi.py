"""
End-to-end smoke test for the SPI's lazy per-module resolution (see
plan/SPI.md), run from a plain launched Python script (jpype.startJVM())
rather than the Java-hosted NGTest harness used by `mvn test` (that
direction is covered by PyLazySpiNGTest).

python.io.PyIoWrapperService registers _io.StringIO with `lazy: true` in
its .pyspi header (unlike BytesIO, which stays eager) - so unlike
BytesIO, nothing should import _io or touch _jpype._concrete for
StringIO until a StringIO instance actually crosses into Java. This is
the "no explicit invocation" property from plan/SPI.md: adding a class
under WrapperService#getResources() with lazy: true is the only thing a
provider has to do; the _jpype._cache probe-miss hook in _core.py takes
care of the rest, from either language direction.

See plan/SPI.md.
"""
import io

import jpype
import jpype.imports

jpype.startJVM()

import _jpype  # noqa: E402


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


# --- Registered lazily at startup, not yet resolved ---

check("_io" in _jpype._lazy_pending and "StringIO" in _jpype._lazy_pending["_io"],
      "expected _io.StringIO to be pending lazy registration before first use, "
      f"got: {_jpype._lazy_pending!r}")

print("OK: _io.StringIO is lazily pending before first use")

# --- First touch, from the Python-launched-script direction, with no
# explicit SPI call anywhere in this script ---

src = io.StringIO("hello")
wrapped = jpype.JObject(src, "python.io.PyStringIO")

check(str(wrapped.getvalue()) == "hello",
      f"unexpected value from lazily-resolved PyStringIO: {wrapped.getvalue()!r}")

print("OK: raw io.StringIO resolved to python.io.PyStringIO with no explicit invocation")

# --- Resolution actually happened - pending entry is drained ---

check("StringIO" not in _jpype._lazy_pending.get("_io", {}),
      f"expected _io.StringIO to be drained from _lazy_pending after first use, "
      f"got: {_jpype._lazy_pending!r}")

print("OK: _lazy_pending was drained after resolution")

jpype.shutdownJVM()
print("test_lazy_spi.py: all checks passed")
