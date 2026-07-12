# Variables
PYTHON := python3
PIP := $(PYTHON) -m pip
IVY_VER := 2.5.3
IVY_JAR := lib/ivy-$(IVY_VER).jar
JAR_SRC  := native/build/lib/org.jpype.jar
JAR_DEST := org.jpype.jar

# Sources
PY_SRC := $(shell find jpype -name "*.py" 2>/dev/null)
CPP_SRC := $(shell find native -name "*.cpp" -o -name "*.h" 2>/dev/null)
JAVA_SRC := $(shell find native/jpype_module/src/main/java -name "*.java" 2>/dev/null)
SENTINEL := .build_history

.PHONY: all clean compile test-java test-python jar

# Default target
all: resolve $(SENTINEL)

resolve: $(IVY_JAR)
	@echo "Resolving dependencies via Ivy..."
	java -jar $(IVY_JAR) -ivy ivy.xml -retrieve 'lib/[artifact]-[revision](-[classifier]).[ext]'

$(IVY_JAR):
	@mkdir -p lib
	@echo "Downloading Ivy..."
	wget --no-clobber "https://repo1.maven.org/maven2/org/apache/ivy/ivy/$(IVY_VER)/ivy-$(IVY_VER).jar" -P lib

# Build the JAR using Ant
org.jpype.jar: $(JAVA_SRC)
	@echo "Building core JAR via Ant..."
	cd native && ant
	cp $(JAR_SRC) $(JAR_DEST)

jar: org.jpype.jar

$(SENTINEL): pyproject.toml $(PY_SRC) $(CPP_SRC) org.jpype.jar
	@echo "Changes detected. Rebuilding project..."
	$(PIP) install -v --no-build-isolation -e . \
		--config-settings=cmake.define.BUILD_TEST_HARNESS=ON \
        --config-settings=cmake.define.CMAKE_BUILD_TYPE=RelWithDebInfo \
		--config-settings=cmake.verbose=true
	@# pyproject.toml builds each Python version into its own build/{wheel_tag}
	@# dir (editable.mode=redirect) so switching versions never reuses another
	@# version's stale CMakeFiles/ object cache. Also drop an ABI-tagged copy
	@# at the repo root (same convention CPython itself uses for compiled
	@# extensions, e.g. _jpype.cpython-310-x86_64-linux-gnu.so) so multiple
	@# Python versions' builds can coexist at root without clobbering each
	@# other - matches the old versioned-.so layout this repo used to have.
	@# _jpyne has no PyInit__jpyne (it's not meant to be `import`ed from
	@# Python - nothing in jpype/ or the Java side does), so it can't be
	@# located via `import _jpyne; .__file__` the way _jpype can. Locate both
	@# by this version's build-dir tag (cp310, cp312, ...) instead of mtime -
	@# an incremental no-op ninja rebuild doesn't touch the output's mtime,
	@# so "newest file" is unreliable when switching back to a version whose
	@# build dir already existed from an earlier session.
	@EXT_SUFFIX=$$($(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"); \
	PYTAG=$$($(PYTHON) -c "import sys; print('cp%d%d' % sys.version_info[:2])"); \
	JP_FILE=$$(echo build/$$PYTAG-*/_jpype.so); \
	JN_FILE=$$(echo build/$$PYTAG-*/_jpyne.so); \
	cp "$$JP_FILE" "_jpype$$EXT_SUFFIX"; \
	cp "$$JN_FILE" "_jpyne$$EXT_SUFFIX"; \
	echo "Copied $$JP_FILE -> _jpype$$EXT_SUFFIX"
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


