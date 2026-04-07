# Variables
PYTHON := python3
PIP := $(PYTHON) -m pip
# Get the platform-specific extension (e.g., .cpython-310-x86_64-linux-gnu.so)
EXT_SUFFIX := $(shell $(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX') or '.so')")

PY_SRC := $(shell find jpype -name "*.py" 2>/dev/null)
CPP_SRC := $(shell find native -name "*.cpp" -o -name "*.h" 2>/dev/null)
SENTINEL := .build_history

.PHONY: all clean 

all: $(SENTINEL)

$(SENTINEL): pyproject.toml $(PY_SRC) $(CPP_SRC)
	@echo "Building for suffix: $(EXT_SUFFIX)"
	@# Use --no-build-isolation to ensure it uses the current environment's headers
	$(PIP) install -v --no-build-isolation -e . 
	@touch $(SENTINEL)
	@echo "Build complete."

clean:
	rm -rf $(BUILD_DIR) dist/ *.egg-info .build_history
	rm -f $(SENTINEL)
	find . -name "*.pyc" -delete
	find . -name "*.so" -delete

