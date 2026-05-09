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
 * <p>
 * Note: This class is intended for internal use and should not be used directly
 * by external code.</p>
 */
class JPypeReference extends PhantomReference<Object>
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
  public JPypeReference(ReferenceQueue<Object> arg1, Object javaObject, long host, long cleanup)
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
    if (!(arg0 instanceof JPypeReference))
      return false;
    return ((JPypeReference) arg0).hostReference == hostReference;
  }
}
