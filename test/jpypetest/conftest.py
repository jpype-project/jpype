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

my_options = {}


def pytest_addoption(parser):
    parser.addoption('--classpath', action="store", default=None,
                     help="Use a jar rather than the thunks.")
    parser.addoption('--convertStrings', action="store_true",
                     default=False, help="Give convert strings to startJVM.")
    parser.addoption('--jacoco', action="store_true",
                     default=False, help="Add Java coverage tool.")
    parser.addoption('--checkjni', action="store_true",
                     default=False, help="Enable JNI checking.")
    parser.addoption('--fast', action="store_true",
                     default=False, help="Skip subrun, jedi and SQL tests.")


def pytest_collection_modifyitems(config, items):
    if config.getoption("--fast"):
        common.fast = True


def start_test_jvm(checkjni=False, classpath=None, convertStrings=False, jacoco=False):
    """Starts a JVM with testing arguments."""
    import jpype
    import _jpype
    from pathlib import Path

    import logging
    try:
        import faulthandler
        faulthandler.enable()
        faulthandler.disable()
    except: ## tODO: too broad exception handling
        pass
    root = Path(__file__).parent.parent
    test_java_classes = root / "classes"
    assert test_java_classes.exists()
    jpype.addClassPath(test_java_classes.absolute())
    jvm_path = jpype.getDefaultJVMPath()

    logger = logging.getLogger(__name__)
    logger.info("Running testsuite using JVM %s" % jvm_path)
    classpath_arg = "-Djava.class.path=%s"
    args = ["-ea", "-Xmx256M", "-Xms16M"]
    if checkjni:
        args.append("-Xcheck:jni")
    if classpath:
        import warnings
        # This needs to be relative to run location
        jpype.addClassPath(Path(classpath).resolve())
        warnings.warn("using jar instead of thunks")
    if convertStrings:
        import warnings
        warnings.warn("using deprecated convertStrings")
    if jacoco:
        import warnings
        args.append(
            "-javaagent:lib/org.jacoco.agent-0.8.5-runtime.jar=destfile=build/coverage/jacoco.exec,includes=org.jpype.*")
        warnings.warn("using JaCoCo")
    jpype.addClassPath(root / "../lib/*")
    jpype.addClassPath(root /  "jar/*")
    classpath_arg %= jpype.getClassPath()
    args.append(classpath_arg)
    _jpype.enableStacktraces(True)
    jpype.startJVM(jvm_path, *args,
                   convertStrings=convertStrings)

@pytest.fixture(scope="session")
def start_test_jvm_per_session(request):
    """Passes the custom pytest arguments for classpath etc. to the testing session JVM."""
    classpath = request.config.getoption("--classpath")
    convertStrings = request.config.getoption("--convertStrings")
    jacoco = request.config.getoption("--jacoco")
    checkjni = request.config.getoption("--checkjni")

    my_options["classpath"] = classpath
    my_options["convertStrings"] = convertStrings
    my_options["jacoco"] = jacoco
    my_options["checkjni"] = checkjni

    start_test_jvm(checkjni=checkjni, classpath=classpath, convertStrings=convertStrings, jacoco=jacoco)
