java  \
    -Djava.util.logging.config.file=logging.properties  \
    -Djpype.nocache=true \
    -Dpython.executable=python3.12  \
    -Dpython.config.home=/usr  \
    -Djpype.path=../..  \
    -cp target/test-classes/  \
    --enable-native-access=org.jpype \
    --module-path=target/jpype-1.7.2.dev0.jar  \
    --add-modules=org.jpype runner/HelloWorldMain \

