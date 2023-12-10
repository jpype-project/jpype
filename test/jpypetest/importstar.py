# This is a short test to see what happens when we import using star.
#
#   Unfortunately this can only be done from a module level so we
#   will need to include a special module for this test.
#
#   When we start the test late2.jar will not be in the current classpath.
#   But we will add it later and make sure the directory updates for us.
import jpype

# import with star the first time
from org.jpype import *  # type: ignore

try:
    # This should not be found
    late2.Test()  # type: ignore[name-defined]
    raise ImportError("late was already found")
except NameError:
    pass

# Path is relative to this module
jpype.addClassPath("../jar/late/late2.jar")

# Second import
if True:
    from org.jpype import *  # type: ignore[name-defined]

# This time it should work
t = late2.Test()  # type: ignore[name-defined]
