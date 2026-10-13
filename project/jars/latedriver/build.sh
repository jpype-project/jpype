javac --release 8 -d classes src/org/jpype/latedriver/*.java
mkdir -p classes/META-INF/services
cp src/META-INF/services/java.sql.Driver classes/META-INF/services/java.sql.Driver
jar --create --file latedriver.jar -C classes .
