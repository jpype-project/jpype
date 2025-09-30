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
import os

import pytest

import common
import jpype


def pytest_addoption(parser):
    parser.addoption('--classpath', action="store", default=None,
                     help="Use a jar rather than the thunks")
    parser.addoption('--convertStrings', action="store_true",
                     default=False, help="Give convert strings to startJVMs")
    parser.addoption('--jacoco', action="store_true",
                     default=False, help="Add Java coverage tool")
    parser.addoption('--checkjni', action="store_true",
                     default=False, help="Enable JNI checking")
    parser.addoption('--fast', action="store_true",
                     default=False, help="Skip subrun tests")


def pytest_collection_modifyitems(config, items):
    if config.getoption("--fast"):
        common.fast = True


@pytest.fixture(scope="class")
def common_opts(request):
    request.cls._classpath = request.config.getoption("--classpath")
    request.cls._convertStrings = request.config.getoption("--convertStrings")
    request.cls._jacoco = request.config.getoption("--jacoco")
    request.cls._checkjni = request.config.getoption("--checkjni")

def _run_cmd(queue, jvm_path):
    import _jpype, pathlib
    print("pid:", os.getpid())
    version = _jpype._get_jvm_version(jvm_path)
    queue.put(version)

@pytest.fixture(scope="session", autouse=True)
def java_version():
    print("pid:", os.getpid())
    print("hi from java version")
    def _java_version() -> dict:
        import jpype
        jvm_path = jpype.getDefaultJVMPath()


        import multiprocessing as mp
        ctx = mp.get_context("spawn")
        queue = ctx.Queue()
        p = ctx.Process(target=_run_cmd, args=(queue, jvm_path))
        p.start()
        print("join version proc")
        p.join(timeout=1)
        print("joinED version proc")
        version = queue.get()
        print(version)

        def parse_output(output):
            import re

            # Look for first quoted string like "25.0.1" or "11.0.20"
            m = re.search(r'"([\d._]+)"', output)
            if m:
                version_str = m.group(1)
                # Normalize underscores to dots
                version_str = version_str.replace("_", ".")
                # Split into components if needed
                parts = version_str.split(".")
                major = int(parts[0])
                minor = int(parts[1]) if len(parts) > 1 else 0
                patch = int(parts[2]) if len(parts) > 2 else 0
                return {"full": version_str, "major": major, "minor": minor, "patch": patch}
            else:
                raise ValueError("Cannot parse java -version output")

        return parse_output(version)

    return _java_version()