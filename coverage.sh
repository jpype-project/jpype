#!/bin/sh

# Script for testing coverage locally
PYTHON=python
PIP="python -m pip"
mvn -q -f project/coverage package
$PYTHON setup.py test_java
$PIP install gcovr pytest-cov jedi
$PYTHON setup.py --enable-coverage --enable-build-jar build_ext --inplace
$PYTHON -m pytest -rsx -v test/jpypetest --cov=jpype --cov-report=xml:coverage_py.xml --jar="build/classes" --jacoco --checkjni
java -jar project/coverage/org.jacoco.cli-0.8.5-nodeps.jar report jacoco.exec --classfiles build/classes/ --html coverage_java --sourcefiles native/java
gcovr -r . --html-details -o coverage/coverage.html --exclude-unreachable-branches --exclude-throw-branches
