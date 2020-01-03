import jpype

# Start up the JVM with coverage instrumentation
jpype.startJVM("-javaagent:project/coverage/org.jacoco.agent-0.8.5-runtime.jar=destfile=jacoco.exec,includes=org.jpype.*:jpype.*,classdumpdir=dump", 
        classpath=["native/org.jpype.jar", "test/classes/"])

# Execute some paths
print(jpype.JString("hello"))

# Force a report from jacoco (Not sure why shutdown isn't trigger this)
RT=jpype.JClass("org.jacoco.agent.rt.RT")
agent=RT.getAgent()
print(agent.getSessionId())
agent.dump(False)

# Shutdown the JVM
jpype.shutdownJVM()

