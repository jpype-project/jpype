javac --release 8 -d classes src/org/jpype/sample_package/*.java
jar --create --file sample_package.jar -C classes .
