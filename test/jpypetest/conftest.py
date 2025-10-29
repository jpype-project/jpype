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

@pytest.fixture(scope="session")
def jvm_session(request):
    """Starts a JVM with testing jars on classpath and additional options."""
    import _jpype
    from pathlib import Path
    import logging
    import warnings

    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger(__name__)

    assert not jpype.isJVMStarted()
    try:
        import faulthandler
        faulthandler.enable()
        faulthandler.disable()
    except:
        pass


    _classpath = request.config.getoption("--classpath")
    _convertStrings = request.config.getoption("--convertStrings")
    _jacoco = request.config.getoption("--jacoco")
    _checkjni = request.config.getoption("--checkjni")

    root = Path(__file__).parent.resolve()
    print("root: %s", root)
    jpype.addClassPath(root / '../classes')
    jvm_path = jpype.getDefaultJVMPath()
    logger.info("Running testsuite using JVM %s" % jvm_path)
    classpath_arg = "-Djava.class.path=%s"
    args = ["-ea", "-Xmx256M", "-Xms16M"]
    if _checkjni:
        args.append("-Xcheck:jni")
    # TODO: enabling this check crashes the JVM with: FATAL ERROR in native method: Bad global or local ref passed to JNI
    # "-Xcheck:jni",
    if _classpath:
        # This needs to be relative to run location
        jpype.addClassPath(Path(_classpath).resolve())
        warnings.warn("using jar instead of thunks")
    if _convertStrings:
        warnings.warn("using deprecated convertStrings")
    if _jacoco:
        args.append(
            "-javaagent:lib/org.jacoco.agent-0.8.5-runtime.jar=destfile=build/coverage/jacoco.exec,includes=org.jpype.*")
        warnings.warn("using JaCoCo")

    jpype.addClassPath(root / "../../lib/*")  # jars downloaded by ivy to root lib directory.
    jpype.addClassPath(root / "../jar/*") # jars in test directory.
    classpath_arg %= jpype.getClassPath()
    args.append(classpath_arg)
    _jpype.enableStacktraces(True)
    jpype.startJVM(jvm_path, *args,
                   convertStrings=_convertStrings)