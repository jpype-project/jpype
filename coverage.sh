#!/bin/sh

# Script for testing coverage locally
PYTHON=python
PIP="python -m pip"
./resolve.sh
$PYTHON setup.py test_java
$PIP install gcovr pytest-cov jedi
$PYTHON setup.py --enable-coverage --enable-build-jar build_ext --inplace
$PYTHON -m pytest -rsx -v test/jpypetest \
	--cov-report=xml:coverage_py.xml \
	--cov-report=html:build/coverage/python \
	--cov=jpype \
	--classpath="build/classes" --jacoco --checkjni
java -jar project/coverage/org.jacoco.cli-0.8.5-nodeps.jar report build/coverage/jacoco.exec \
	--classfiles build/classes/ \
	--html build/coverage/java \
	--sourcefiles native/java
gcovr -r . --html-details -o build/coverage/cpp/jpype.html --exclude-unreachable-branches --exclude-throw-branches
