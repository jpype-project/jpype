/* ****************************************************************************
  Copyright 2019, Karl Einar Nelson

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 *****************************************************************************/
package org.jpype;

import org.jpype.manager.TypeFactory;
import org.jpype.manager.TypeFactoryNative;
import org.jpype.manager.TypeManager;
import org.jpype.ref.JPypeReferenceQueue;

/**
 * Context for JPype.
 * <p>
 * This is the part of JPype that holds all resources. After the classloader is
 * created this class is given the address of the context object in JPype. Any
 * resources in JPype Java layer can be contacted using the context.
 * <p>
 * Boot order is - create the C++ portion of the context. - start the jvm - load
 * the bootloader - install the jar into the bootloader - install all native
 * methods using the bootloader - create the Java portion of the context. - use
 * the Java context to access the resources (ReferenceQueue, TypeFactory,
 * TypeManager)
 * <p>
 * Once started, python calls use the context to get a frame and attach their
 * threads. Methods called from Java will get the env and use it to get their
 * context from which they can create a frame.
 * <p>
 * The C++ context will hold all the previous global variables thus allowing the
 * C++ portion to be cleaned up properly when the JVM is shutdown or
 * disconnected.
 * <p>
 * As the JPypeContext can't be tested directly from Java code, it will need to
 * be kept light.
 * <p>
 * Our goal is to remove as much direct contact methods as possible from the C++
 * layer. Previous globals in JPTypeManager move to the context as do the
 * contents of JPJni.
 *
 *
 *
 * @author nelson85
 */
public class JPypeContext
{
  private static JPypeContext instance = null;
  // This is the C++ portion of the context.
  private long context;
  private TypeFactory typeFactory;
  private TypeManager typeManager;
  private JPypeReferenceQueue referenceQueue;
  private ClassLoader bootLoader;

  static public JPypeContext getInstance()
  {
    return instance;
  }

  /**
   * Start the JPype system.
   *
   * @param context
   */
  public static JPypeContext createContext(long context, ClassLoader bootLoader)
  {
    instance = new JPypeContext();

    instance.context = context;
    instance.bootLoader = bootLoader;
    instance.typeFactory = new TypeFactoryNative();
    instance.typeManager = new TypeManager(context, instance.typeFactory);
    instance.typeManager.typeFactory = instance.typeFactory;

    instance.referenceQueue = new JPypeReferenceQueue(context);

    // Okay everything is setup so lets give it a go.
    instance.typeManager.init();
    instance.referenceQueue.start();

    return instance;
  }

  public void shutdown()
  {
    this.referenceQueue.stop();
    this.typeManager.shutdown();
  }

  /**
   * Get the C++ portion.
   *
   * @return
   */
  public long getContext()
  {
    return context;
  }

  public ClassLoader getBootLoader()
  {
    return this.bootLoader;
  }

  public TypeFactory getTypeFactory()
  {
    return this.typeFactory;
  }

  public TypeManager getTypeManager()
  {
    return this.typeManager;
  }

  public JPypeReferenceQueue getReferenceQueue()
  {
    return this.referenceQueue;
  }
}
