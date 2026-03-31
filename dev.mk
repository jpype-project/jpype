# Variables
PYTHON := python3
PIP := $(PYTHON) -m pip

# Adjusted for JPype's actual structure
PY_SRC := $(shell find jpype -name "*.py" 2>/dev/null)
CPP_SRC := $(shell find native -name "*.cpp" -o -name "*.h" 2>/dev/null)
SENTINEL := .build_history
BUILD_DIR := target

.PHONY: all clean compile

# Default target
all: $(SENTINEL)

# The sentinel depends on pyproject and all source files
$(SENTINEL): pyproject.toml $(PY_SRC) $(CPP_SRC)
	@echo "Changes detected. Rebuilding project..."
	@# We skip 'python -m build' because pip install -e does the heavy lifting
	$(PIP) install -v --no-build-isolation -e . \
		--config-settings=build.dir=$(BUILD_DIR) \
		--config-settings=cmake.verbose=true
	@touch $(SENTINEL)
	@echo "Build complete."

clean:
	rm -rf $(BUILD_DIR) dist/ *.egg-info .build_history
	rm -f $(SENTINEL)
	find . -name "*.pyc" -delete
	find . -name "*.so" -delete

