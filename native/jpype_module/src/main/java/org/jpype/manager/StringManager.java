// --- file: org/jpype/manager/StringManager.java ---
package org.jpype.manager;

import org.jpype.internal.NativeContext;

/**
 *
 * @author nelson85
 */
public class StringManager
{
  NativeContext context;
  long address;
  
  public StringManager(NativeContext context)
  {
    this.context = context;
    this.address = context.address();
  }
  
  public long get(String name)
  {
    return impl(address, name);
  }
  
  public native static long impl(long context, String name);
}
