import os

import jpype
import jpype.imports
from jpype.types import *

for key, value in sorted(os.environ.items()):
    print(f'{key!r}: {value!r}')

print('+++ about to start JVM')
jpype.startJVM()
print('+++ JVM started')
