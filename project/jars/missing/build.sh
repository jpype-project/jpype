javac --release 7 -d classes src/org/jpype/missing/*.java
jar --create --file missing.jar -M -C classes META-INF/MANIFEST.MF -C classes org/jpype/missing/Test.class
