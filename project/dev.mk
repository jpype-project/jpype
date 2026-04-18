# Variables
PYTHON := python3
PIP := $(PYTHON) -m pip
IVY_VER := 2.5.3
IVY_JAR := lib/ivy-$(IVY_VER).jar

# Sources
PY_SRC := $(shell find jpype -name "*.py" 2>/dev/null)
CPP_SRC := $(shell find native -name "*.cpp" -o -name "*.h" 2>/dev/null)
SENTINEL := .build_history

.PHONY: all clean compile test-java test-python

# Default target
all: resolve $(SENTINEL)

resolve: $(IVY_JAR)
	@echo "Resolving dependencies via Ivy..."
	java -jar $(IVY_JAR) -ivy ivy.xml -retrieve 'lib/[artifact]-[revision](-[classifier]).[ext]'

$(IVY_JAR):
	@mkdir -p lib
	@echo "Downloading Ivy..."
	wget --no-clobber "https://repo1.maven.org/maven2/org/apache/ivy/ivy/$(IVY_VER)/ivy-$(IVY_VER).jar" -P lib


$(SENTINEL): pyproject.toml $(PY_SRC) $(CPP_SRC)
	@echo "Changes detected. Rebuilding project..."
	$(PIP) install -v --no-build-isolation -e . \
		--config-settings=cmake.define.BUILD_TEST_HARNESS=ON \
		--config-settings=cmake.verbose=true \
		--config-settings=editable.mode=inplace
	@touch $(SENTINEL)

# This target ensures the harness is built before running pytest
test: test-java test-python

test-java:
	@echo "Building Java Test Harness via Ant..."
	ant -f test

test-python:
	@echo "Running Pytest..."
	# We cd into test just like the Azure runner to avoid path confusion
	cd test && $(PYTHON) -m pytest -v jpypetest --checkjni

clean:
	@echo "Cleaning up build artifacts..."
	# Remove directories
	rm -rf dist/ *.egg-info CMakeFiles/ .pytest_cache/
	# Remove scikit-build / CMake / Ninja metadata from root
	rm -f .ninja_deps .ninja_log .skbuild-info.json
	rm -f CMakeCache.txt CMakeInit.txt build.ninja cmake_install.cmake
	rm -f Makefile cmake_install.cmake  # In case it used Unix Makefiles
	# Remove build products
	rm -f _jpype.so org.jpype.jar $(SENTINEL)
	# Recursive cleanup
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -type d -exec rm -rf {} +
	@echo "Clean complete."


