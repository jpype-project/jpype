import pytest

def pytest_addoption(parser):
    parser.addoption('--jar', action="store", default=None, help="Use a jar rather than the thunks")
    parser.addoption('--convertStrings', action="store_true", default=False, help="Give convert strings to startJVMs")
    parser.addoption('--jacoco', action="store_true", default=False, help="Add Java coverage tool")

@pytest.fixture(scope="class")
def common_opts(request):
    request.cls._jar = request.config.getoption("--jar")
    request.cls._convertStrings = request.config.getoption("--convertStrings")
    request.cls._jacoco = request.config.getoption("--jacoco")

