// --- file: org/jpype/proxy/ProxyInstance.java ---
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Map;
import org.jpype.internal.NativeContext;
import python.lang.PyBuiltIn;
import python.lang.PyKwArgs;

public class ProxyInstance implements InvocationHandler
{

//  static final TypeManager manager = JPypeContext.getInstance().getTypeManager();
  private final ProxyType type;
  final long instance; // JPProxy*

  public ProxyInstance(ProxyType type, long instance)
  {
    this.type = type;
    this.instance = instance;
  }

  public ProxyType getType()
  {
    return type;
  }

  @Override
  public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
  {
    NativeContext context = type.context;
    if (context.isShutdown())
      throw new RuntimeException("Proxy called during shutdown");

    MethodDescriptor md = type.getMethodDescriptor(method);
    if (md.bypass)
      return md.defaultHandler.bindTo(proxy).invokeWithArguments(args);

    // Flatten varargs and split off a trailing PyKwArgs marker (if any) so
    // the downcall array is [positional..., key0, value0, key1, value1, ...].
    Object[] flat = args;
    int posCount = 0;
    int kwCount = 0;

    if (args != null)
    {
      posCount = args.length;
      if (method.isVarArgs() && posCount > 0 && args[posCount - 1] instanceof Object[])
      {
        // Reflect.Proxy packs the varargs tail into a single Object[]
        // element; unpack it so a caller-supplied PyKwArgs (always the
        // last variadic element) can be found and spliced out below.
        Object[] varargs = (Object[]) args[posCount - 1];
        int fixedCount = posCount - 1;
        posCount = fixedCount + varargs.length;
        flat = new Object[posCount];
        System.arraycopy(args, 0, flat, 0, fixedCount);
        System.arraycopy(varargs, 0, flat, fixedCount, varargs.length);
      }

      if (posCount > 0 && flat[posCount - 1] instanceof PyKwArgs)
      {
        PyKwArgs kw = (PyKwArgs) flat[posCount - 1];
        posCount--;
        kwCount = kw.size();
        Object[] combined = new Object[posCount + 2 * kwCount];
        System.arraycopy(flat, 0, combined, 0, posCount);
        int idx = posCount;
        for (Map.Entry<String, Object> e : kw.entrySet())
        {
          combined[idx++] = e.getKey();
          combined[idx++] = e.getValue();
        }
        flat = combined;
      }
    }

    int total = flat == null ? 0 : flat.length;
    long[] scratch = get(total);
    for (int i = 0; i < total; ++i)
    {
      long cls = type.typeManager.findClassForObject(flat[i]);
      if (cls == 0L && md.parameterTypes.length > 0)
        cls = md.parameterTypes[Math.min(i, md.parameterTypes.length - 1)];
      scratch[i] = cls;
    }

    // Resolve method parameter and return types
    // The type resolution logic remains, but uses the shared context
    Object result = hostInvoke(md.name, instance, md.returnType, scratch, flat, posCount, kwCount);

    if (result != scratch)
      return result;

    // FIXME in Java 16 they made it possible to call Default, once we abandon 9 we can safely run it.
    //    return InvocationHandler.invokeDefault(proxy, method, args);
    // Handle default methods in interfaces
    if (md.defaultHandler != null)
      return md.defaultHandler.bindTo(proxy).invokeWithArguments(args);

    throw new NoSuchMethodError(method.getName());
  }

  private static final int INITIAL_SIZE = 16;

  private static final ThreadLocal<long[]> CACHE = ThreadLocal.withInitial(() -> new long[INITIAL_SIZE]);

  /**
   * Ensures the current thread's cache is at least 'requiredSize'. Returns the
   * (possibly new) array.
   *
   * @param requiredSize
   * @return
   */
  public static long[] get(int requiredSize)
  {
    long[] current = CACHE.get();
    if (current.length < requiredSize)
    {
      long[] next = new long[requiredSize];
      CACHE.set(next);
      return next;
    }
    return current;
  }

  /**
   * Native method to invoke a method on the Python object.
   *
   * @param name is the name of the method to invoke.
   * @param pyObject is the instance ID of the Python object.
   * @param returnType is the return type of the method.
   * @param argsTypes is the types of the method parameters, indexed the same
   * as {@code args}.
   * @param args is the positional arguments followed by, if
   * {@code numKeyword > 0}, {@code numKeyword} flattened
   * {@code (String key, Object value)} pairs
   * (length == {@code numPositional + 2 * numKeyword}).
   * @param numPositional is the number of leading positional arguments in
   * {@code args}.
   * @param numKeyword is the number of trailing keyword-argument pairs in
   * {@code args}.
   * @return the result of the method invocation or argsTypes on a missing impl.
   */
  private static native Object hostInvoke(long name, long pyObject,
          long returnType, long[] argsTypes, Object[] args, int numPositional, int numKeyword);

    
  public static PyBuiltIn get(Object obj)
  {
    return ((ProxyInstance) Proxy.getInvocationHandler(obj)).type.builtin;
  }

}
