import pytest
import compile_java_classes

compile_java_classes.build_all('test', 'test/harness')

args = ['test/jpypetest', '-vv']

pytest.main(args)
