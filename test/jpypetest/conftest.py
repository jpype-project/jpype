import pytest
import jpype
import common


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
