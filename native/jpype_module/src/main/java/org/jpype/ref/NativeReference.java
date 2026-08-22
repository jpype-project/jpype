// --- file: org/jpype/ref/NativeReference.java ---
/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.ref;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.lang.reflect.Method;

/**
 * Represents a phantom reference to a Python object in JPype.
 *
 * <p>
 * This class is used internally by JPype to manage references to Python objects
 * (`PyObject*`) from Java. It extends {@link PhantomReference} to enable
 * cleanup operations when the referenced Java object is garbage collected.</p>
 *
 * <p>
 * Each instance of {@code JPypeReference} holds metadata about the Python
 * object, including its host reference and cleanup function ID. These
 * references are managed using a {@link ReferenceQueue} to facilitate resource
 * cleanup.</p>
 *
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
 *
 *
 * <p>
 * Note: This class is intended for internal use and should not be used directly
 * by external code.</p>
 */
class NativeReference extends PhantomReference<Object>
{

  /**
   * The host reference to the Python object.
   * <p>
   * This is a native pointer to the Python object (`PyObject*`).</p>
   */
  long hostReference;

  /**
   * The cleanup function ID for the Python object.
   * <p>
   * This function is invoked to release resources associated with the Python
   * object.</p>
   */
  long cleanup;

  /**
   * The pool ID associated with this reference.
   * <p>
   * Used internally to manage pooled resources.</p>
   */
  int pool;

  /**
   * The index of this reference within the pool.
   * <p>
   * Used internally for efficient resource management.</p>
   */
  int index;

  /**
   * Constructs a new {@code JPypeReference}.
   *
   * @param arg1 The {@link ReferenceQueue} to which this reference will be
   * registered.
   * @param javaObject The Java object being referenced.
   * @param host The host reference to the Python object (`PyObject*`).
   * @param cleanup The cleanup function ID for the Python object.
   */
  public NativeReference(ReferenceQueue<Object> arg1, Object javaObject, long host, long cleanup)
  {
    super(javaObject, arg1);
    this.hostReference = host;
    this.cleanup = cleanup;
  }

  /**
   * Computes the hash code for this reference.
   *
   * <p>
   * The hash code is derived from the {@code hostReference} field to ensure
   * consistency with the {@link #equals(Object)} method.</p>
   *
   * @return The hash code for this reference.
   */
  @Override
  public int hashCode()
  {
    return (int) hostReference;
  }

  /**
   * Compares this reference to another object for equality.
   *
   * <p>
   * Two {@code JPypeReference} objects are considered equal if their
   * {@code hostReference} fields are identical.</p>
   *
   * @param arg0 The object to compare with this reference.
   * @return {@code true} if the objects are equal; {@code false} otherwise.
   */
  @Override
  public boolean equals(Object arg0)
  {
    if (!(arg0 instanceof NativeReference))
      return false;
    return ((NativeReference) arg0).hostReference == hostReference;
  }

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
  public static native void removeHostReference(long ctx, long host, long cleanup);

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
  public static native void wake(long ctx);

  /**
   * Initializes resources for an object.
   *
   * This method sets up the necessary native resources for the specified object
   * and associates it with a method. It is typically called during the object's
   * creation or initialization phase.
   *
   * @param ctx The address of the owning native JPContext.
   * @param self The Java object that requires resource initialization.
   * @param m The {@link Method} object representing the method to associate
   * with the object.
   *
   */
  public static native void init(long ctx, Object self, Method m);

}
