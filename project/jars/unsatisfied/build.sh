javac --release 7 -d classes src/org/jpype/unsatisfied/*.java
rm classes/org/jpype/unsatisfied/UnsatisfiedClass.class
jar --create --file unsatisfied.jar -C classes . 
