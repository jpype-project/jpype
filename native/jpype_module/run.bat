java  ^
   -Djava.util.logging.config.file=logging.properties ^
   -Djpype.nocache=true ^
   -cp target\test-classes ^
   --enable-native-access=org.jpype ^
   --module-path=target\jpype-1.7.2.dev0.jar  ^
   --add-modules=org.jpype runner/HelloWorldMain 

