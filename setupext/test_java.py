# -*- coding: utf-8 -*-
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
import sys
import os
import subprocess
import distutils.cmd
import distutils.log
import glob
import re
import shlex


def getJavaVersion(javac):
    # Find Java version
    version_str = os.popen('"%s" -version' % javac).read()
    result = re.match(r'javac ([0-9]+)\.([0-9]+)\..*', version_str)
    if not result:
        return 8
    if int(result.group(1)) > 1:
        return int(result.group(1))
    return int(result.group(2))


def compileJava():
    javac = "javac"
    try:
        javac0 = os.path.join(os.environ['JAVA_HOME'], 'bin', 'javac')
        if os.name == "nt":
            javac0 += ".exe"
        if os.path.exists(javac0):
            javac = javac0
    except KeyError:
        print("No JAVA_HOME set")
        pass
    print("JAVAC =", javac)
    version = getJavaVersion(javac)
    srcs = glob.glob('test/harness/jpype/**/*.java', recursive=True)
    srcs.extend(glob.glob('test/harness/org/**/*.java', recursive=True))
    exports = ""
    if version > 7:
        srcs.extend(glob.glob('test/harness/java8/**/*.java', recursive=True))
    if version > 8:
        srcs.extend(glob.glob('test/harness/java9/**/*.java', recursive=True))
        exports = "--add-exports java.base/jdk.internal.reflect=ALL-UNNAMED"
    cmd = shlex.split(
        '"%s" -d test/classes %s -g:lines,vars,source' % (javac, exports))
    os.makedirs("test/classes", exist_ok=True)
    cmd.extend(srcs)
    return cmd


class TestJavaCommand(distutils.cmd.Command):
    """A custom command to create jar file during test."""

    description = 'run javac to make test harness'
    user_options = []

    def initialize_options(self):
        """Set default values for options."""
        pass

    def finalize_options(self):
        """Post-process options."""
        pass

    def run(self):
        """Run command."""
        if os.path.exists(os.path.join("test", "classes")):
            distutils.log.info("Skip building Java testbench")
            return
        cmdStr = compileJava()
        self.announce("  %s" % " ".join(cmdStr), level=distutils.log.INFO)
        subprocess.check_call(cmdStr)
        subprocess.check_call(shlex.split("javadoc test/harness/jpype/doc/Test.java -d test/classes/"))
