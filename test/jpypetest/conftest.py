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

convertStrings = None
_jvm_started_by_conftest = False


def _start_jvm(config):
    """Start the JVM used by the whole test session (once per process)."""
    global convertStrings, _jvm_started_by_conftest
    import _jpype
    from pathlib import Path
    import logging

    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger(__name__)

    assert not jpype.isJVMStarted()

    classpath = config.getoption("--classpath")
    convertStrings = config.getoption("--convertStrings")
    jacoco = config.getoption("--jacoco")
    checkjni = config.getoption("--checkjni")

    root = Path(__file__).parent.resolve()
    classes = root / '../classes'
    assert classes.exists()
    jpype.addClassPath(classes)
    jvm_path = jpype.getDefaultJVMPath()
    logger.info("Running testsuite using JVM %s", jvm_path)
    classpath_arg = "-Djava.class.path=%s"
    args = ["-ea", "-Xmx256M", "-Xms16M",
            #"-Xrs"  # disable signal handler for sigsegv, sigabrt
            ]
    if checkjni:
        args.append("-Xcheck:jni")
    if classpath:
        # This needs to be relative to run location
        jpype.addClassPath(Path(classpath).resolve())
        config.issue_config_time_warning(
            UserWarning("using jar instead of thunks"), stacklevel=2)
    if convertStrings:
        config.issue_config_time_warning(
            UserWarning("using deprecated convertStrings"), stacklevel=2)
    if jacoco:
        lib_dir = (root / '../../lib')
        assert lib_dir.exists()
        jar = (lib_dir / "org.jacoco.agent-0.8.5-runtime.jar").absolute()
        assert jar.exists()
        args.append(
            f"-javaagent:{jar}=destfile=build/coverage/jacoco.exec,includes=org.jpype.*")
        config.issue_config_time_warning(
            UserWarning("using JaCoCo"), stacklevel=2)

    jpype.addClassPath(root / "../../lib/*")  # jars downloaded by ivy to root lib directory.
    jpype.addClassPath(root / "../jar/*")  # jars in test directory.
    classpath_arg %= jpype.getClassPath()
    args.append(classpath_arg)
    _jpype.enableStacktraces(True)
    jpype.startJVM(jvm_path, *args,
                   convertStrings=convertStrings)
    _jvm_started_by_conftest = True


def pytest_sessionstart(session):
    """Start the JVM before any test module is collected.

    Plugins that inspect the tests while collecting (e.g. pytest-run-parallel
    resolves ``java.lang.X`` attribute chains from the test modules' globals)
    would otherwise hit JPype's "Unable to import 'java.lang' without JVM"
    RuntimeError, because the JVM was only started lazily by the first test.

    With pytest-xdist the controller process neither collects nor runs tests,
    so only the processes with ``dist == "no"`` (plain runs and xdist workers)
    start a JVM.
    """
    if getattr(session.config.option, "dist", "no") != "no":
        return
    _start_jvm(session.config)


@pytest.fixture(scope="session")
def jvm_session(request):
    """Makes sure the JVM with the testing jars and options is running.

    Normally it was already started in pytest_sessionstart; this is the fallback
    for when this conftest was loaded too late for that hook to be called.
    """
    if not _jvm_started_by_conftest:
        _start_jvm(request.config)
