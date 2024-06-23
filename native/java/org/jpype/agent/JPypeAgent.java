package org.jpype.agent;

import java.lang.instrument.Instrumentation;

public class JPypeAgent
{
      public static void premain(String agentArgs, Instrumentation inst) {
        System.out.println("Start jpype");
      }
}
