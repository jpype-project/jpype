PYTHONPATH="../.." java -cp target/test-classes/ --module-path=target/jpype-1.7.2.dev0.jar -Dpython.executable=python3.12 -Dpython.config.home=/usr --add-modules=org.jpype runner/HelloWorldMain
# -Djpype.nocache=True
