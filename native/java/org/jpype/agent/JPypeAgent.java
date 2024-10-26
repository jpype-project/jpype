package org.jpype.agent;

import java.lang.instrument.Instrumentation;

public class JPypeAgent
{
      public static void premain(String agentArgs, Instrumentation inst) {
          // This doesn't have to do anything.  
          // We just need to be an agent to load elevated privileges 
      }
}
