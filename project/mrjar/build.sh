javac --release 7 -d classes src/org/jpype/mrjar/*.java
javac --release 7 -d classes src/org/jpype/mrjar/sub/*.java
javac --release 9 -d classes-9 src/java9/org/jpype/mrjar/*.java
jar --create --file mrjar.jar -C classes . --release 9 -C classes-9 .
