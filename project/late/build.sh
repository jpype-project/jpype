javac --release 7 -d classes1 src/org/jpype/late/*.java
javac --release 7 -d classes2 src/org/jpype/late2/*.java
jar --create --file late.jar -C classes1 . 
jar --create --file late2.jar -C classes2 . 
