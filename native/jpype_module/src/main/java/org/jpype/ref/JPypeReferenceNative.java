package org.jpype.ref;

import java.lang.reflect.Method;

/**
 *
 * @author nelson85
 */
public class JPypeReferenceNative
{

  /**
   * Native hook to delete a native resource.
   *
   * @param host is the address of memory in C.
   * @param cleanup is the address the function to cleanup the memory.
   */
  public static native void removeHostReference(long host, long cleanup);

  /**
   * Triggered by the sentinel when a GC starts.
   */
  public static native void wake();

  /**
   * Initialize resources.
   *
   * @param self
   * @param m
   */
  public static native void init(Object self, Method m);

}
