package org.jpype.ref;

import java.lang.reflect.Method;

/**
 * Internal class used to manage object lifetimes in the Python-to-Java bridge.
 *
 * The {@code JPypeReferenceNative} class provides native methods to handle
 * memory management and resource cleanup for objects shared between Python and
 * Java. It acts as a bridge to the underlying C++ layer, enabling efficient
 * management of native resources during garbage collection (GC) and object
 * lifecycle events.
 *
 * <p>
 * Key responsibilities:
 * <ul>
 * <li>Remove references to native resources when they are no longer needed</li>
 * <li>Handle initialization of resources for objects</li>
 * <li>Respond to garbage collection events</li>
 * </ul>
 *
 * <p>
 * This class is primarily intended for internal use within the JPype library
 * and should not be used directly by application developers.
 */
public class JPypeReferenceNative
{

  /**
   * Removes a reference to a native resource.
   *
   * This method is used to release native memory associated with a specific
   * resource. It calls a cleanup function provided by the native layer to
   * deallocate memory or perform other cleanup operations.
   *
   * @param host The address of the memory in the native layer (C/C++) that
   * needs to be cleaned up.
   * @param cleanup The address of the native function responsible for cleaning
   * up the memory.
   *
   */
  public static native void removeHostReference(long host, long cleanup);

  /**
   * Wakes up the native layer during garbage collection.
   *
   * This method is triggered by a sentinel when the garbage collector starts.
   * It ensures that the native layer is aware of GC events and can perform
   * necessary operations to manage object lifetimes.
   *
   * <p>
   * Usage Example:
   * <pre>
   * JPypeReferenceNative.wake();
   * </pre>
   */
  public static native void wake();

  /**
   * Initializes resources for an object.
   *
   * This method sets up the necessary native resources for the specified object
   * and associates it with a method. It is typically called during the object's
   * creation or initialization phase.
   *
   * @param self The Java object that requires resource initialization.
   * @param m The {@link Method} object representing the method to associate
   * with the object.
   *
   *
   */
  public static native void init(Object self, Method m);
}
