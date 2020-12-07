#!/bin/sh
# This is used to pull dependencies needed for this package

# drill has a bunch of dependencies, so we are going to use ivy to collect it all at once.
wget -nc "https://repo1.maven.org/maven2/org/apache/ivy/ivy/2.5.0/ivy-2.5.0.jar" -P lib
java -jar lib/ivy-2.5.0.jar -ivy ivy.xml -retrieve 'lib/[artifact]-[revision](-[classifier]).[ext]'
