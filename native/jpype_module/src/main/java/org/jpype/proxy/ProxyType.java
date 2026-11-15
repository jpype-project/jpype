// --- file: org/jpype/proxy/ProxyType.java ---
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import org.jpype.annotation.Builtin;
import org.jpype.internal.NativeContext;
import org.jpype.manager.StringManager;
import org.jpype.manager.TypeManager;
import org.jpype.annotation.Bypass;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import python.lang.PyBuiltIn;
import python.lang.PyObject;

public final class ProxyType
{

  final PyBuiltIn builtin;
  final NativeContext context;
  final StringManager stringManager;
  final TypeManager typeManager;

  // Static cache for standard Object methods to avoid re-resolving them
  private final Class<?>[] interfaces;
  private final ClassLoader cl;
  final long cleanup;
  final Map<Method, MethodDescriptor> methodCache;
  private final boolean mangle;

  /**
   * Initializes the static cache for Object methods. This happens once when the
   * class is loaded.
   *
   * @param tm
   */
  public void init(TypeManager tm)
  {

  }

  public ProxyType(ProxyFactory factory, long cleanup, Class<?>[] interfaces)
  {

    this.context = factory.context;
    this.typeManager = context.getTypeManager();
    this.stringManager = context.getStringManager();
    this.interfaces = interfaces;
    this.cleanup = cleanup;
    this.builtin = factory.context.getBuiltIn();

    boolean mangle = false;
    for (Class<?> iface : interfaces)
      if (PyObject.class.isAssignableFrom(iface))
      {
        mangle = true;
        break;
      }
    this.mangle = mangle;

    // Pin the loader to the org.jpype module loader as the baseline default
    ClassLoader tempCl = ProxyType.class.getClassLoader();
    if (tempCl == null)
      tempCl = ClassLoader.getSystemClassLoader();

    // Only override if an interface explicitly comes from an independent classloader
    for (Class<?> cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      // Ignore the bootstrap loader (null) and our own loader
      if (icl != null && icl != tempCl && icl != ClassLoader.getSystemClassLoader())
      {
        tempCl = icl;
        break; // Stop if we hit an explicit custom application/plugin loader
      }
    }
    this.cl = tempCl;

    // Build the instance-specific cache
    Map<Method, MethodDescriptor> tempMap = new HashMap<>();

    // 1. Bulk copy the pre-resolved Object methods
    tempMap.putAll(factory.objectMethods);

    // 2. Resolve interface-specific methods
    TypeManager tm = context.getTypeManager();
    synchronized (tm)
    {
      // A method signature (name + parameter types) can be reached through
      // several interfaces in this proxy's interface list (e.g. an abstract
      // declaration inherited from java.util.List alongside a @Bypass default
      // override declared directly on PyTuple). java.lang.reflect.Proxy picks
      // one canonical Method per signature to hand to InvocationHandler.invoke,
      // and it is not guaranteed to be the most-derived/most-specific one - it
      // may be the plain abstract List-declared Method, which carries neither
      // @Bypass nor a default MethodHandle. Track the best descriptor per
      // signature first, then register it under every Method sharing that
      // signature so lookup succeeds regardless of which one the JVM passes.
      Map<MethodKey, Method> bestBySignature = new HashMap<>();
      for (Class<?> iface : interfaces)
      {
        for (Method method : iface.getMethods())
        {
          MethodKey key = new MethodKey(method);
          Method existing = bestBySignature.get(key);
          if (existing == null || isBetter(method, existing))
            bestBySignature.put(key, method);
        }
      }

      Map<MethodKey, MethodDescriptor> descriptors = new HashMap<>();
      for (Map.Entry<MethodKey, Method> entry : bestBySignature.entrySet())
        descriptors.put(entry.getKey(), buildDescriptor(tm, entry.getValue()));

      for (Class<?> iface : interfaces)
        for (Method method : iface.getMethods())
          tempMap.putIfAbsent(method, descriptors.get(new MethodKey(method)));
    }

    this.methodCache = Collections.unmodifiableMap(tempMap);
  }

  // Never settle on a compiler-generated covariant-return bridge (e.g. the
  // synthetic `Object get(int)` bridge javac emits alongside PySequence's
  // real `PyObject get(int)` override - MethodKey ignores return type so the
  // two collide here). A bridge's default body just re-invokes the covariant
  // method via a normal interface call, which routes back through this same
  // proxy dispatch and recurses forever if picked as the "real" descriptor.
  //
  // Otherwise prefer whichever declaring interface is more specific (a
  // subinterface extending the other), matching normal Java override
  // resolution: a subinterface that re-declares a method - whether adding a
  // new default or deliberately reverting to abstract to change dispatch -
  // always wins over an ancestor interface's declaration. For example
  // PyDict.get(Object) is abstract on purpose (routes to Python's real
  // dict.get(), None-returning) and must win over PyMapping's @Bypass
  // default get(Object) (raw `x[key]`, raises on a missing key) even though
  // the latter "looks" more usable by a default/bypass-only heuristic.
  //
  // Only for genuinely unrelated interfaces (a real diamond, e.g. an
  // abstract method inherited unmodified from java.util.List alongside a
  // subinterface's own default) fall back to preferring @Bypass/default
  // over plain abstract.
  private static boolean isBetter(Method candidate, Method current)
  {
    if (current.isBridge() && !candidate.isBridge())
      return true;
    if (candidate.isBridge() && !current.isBridge())
      return false;

    Class<?> candidateClass = candidate.getDeclaringClass();
    Class<?> currentClass = current.getDeclaringClass();
    if (candidateClass != currentClass)
    {
      if (currentClass.isAssignableFrom(candidateClass))
        return true;
      if (candidateClass.isAssignableFrom(currentClass))
        return false;
    }

    boolean candidateGood = candidate.isAnnotationPresent(Bypass.class) || candidate.isDefault();
    boolean currentGood = current.isAnnotationPresent(Bypass.class) || current.isDefault();
    return candidateGood && !currentGood;
  }

  private MethodDescriptor buildDescriptor(TypeManager tm, Method method)
  {
    long returnType = tm.findClass(method.getReturnType());
    Class<?>[] params = method.getParameterTypes();
    long[] paramTypes = new long[params.length];
    for (int i = 0; i < params.length; i++)
      paramTypes[i] = tm.findClass(params[i]);

    boolean bypass = (method.isAnnotationPresent(Bypass.class));
    // equals/hashCode/toString are special-cased by java.lang.reflect.Proxy
    // itself: no matter how a proxy interface redeclares them, invoke() is
    // always called with Object.class's own Method for these three (see
    // ProxyFactory.objectMethods, resolved once from Object.class and never
    // interface-specific) - confirmed empirically, not just from the JDK's
    // ProxyGenerator special-casing them by name. A mangled wireName here
    // would build a descriptor that's simply never looked up at runtime,
    // while ProxyFactory.objectMethods keeps dispatching under the
    // unmangled name - so the Python-side dict entries for these three must
    // stay unmangled too, regardless of `mangle`.
    boolean isObjectMethod = isObjectMethodSignature(method);
    String wireName = (this.mangle && !isObjectMethod) ? mangle(method.getName()) : method.getName();
    if (method.isAnnotationPresent(Builtin.class))
    {
      // Install a magic backdoor for builtin() so that Interface can find its support backend.
      try
      {
        MethodHandle defaultHandle = MethodHandles.lookup().findStatic(ProxyInstance.class, "get", MethodType.methodType(PyBuiltIn.class, Object.class));
        return new MethodDescriptor(this.stringManager.get(wireName), returnType, paramTypes, defaultHandle, true);
      } catch (NoSuchMethodException | IllegalAccessException ex)
      {
        System.getLogger(ProxyType.class.getName()).log(System.Logger.Level.ERROR, (String) null, ex);
      }
    }

    MethodHandle defaultHandle = null;
    if (method.isDefault())
      defaultHandle = getDefaultHandle(method.getDeclaringClass(), method, java.lang.invoke.MethodHandles.class);
    return new MethodDescriptor(this.stringManager.get(wireName), returnType, paramTypes, defaultHandle, bypass);
  }

  // User-defined wrapper methods on a PyObject-rooted interface use a leading
  // `$` (see PyObject's javadoc) so they dispatch directly by their real
  // Python name instead of through the map-only protocol-method namespace.
  private static String mangle(String name)
  {
    if (name.startsWith("$"))
      return name.substring(1);
    return "." + name;
  }

  private static boolean isObjectMethodSignature(Method method)
  {
    String name = method.getName();
    Class<?>[] params = method.getParameterTypes();
    if (name.equals("equals") && params.length == 1 && params[0] == Object.class)
      return true;
    if (name.equals("hashCode") && params.length == 0)
      return true;
    if (name.equals("toString") && params.length == 0)
      return true;
    return false;
  }

  private static final class MethodKey
  {
    private final String name;
    private final Class<?>[] paramTypes;

    MethodKey(Method method)
    {
      this.name = method.getName();
      this.paramTypes = method.getParameterTypes();
    }

    @Override
    public boolean equals(Object o)
    {
      if (!(o instanceof MethodKey))
        return false;
      MethodKey other = (MethodKey) o;
      return name.equals(other.name) && java.util.Arrays.equals(paramTypes, other.paramTypes);
    }

    @Override
    public int hashCode()
    {
      return name.hashCode() * 31 + java.util.Arrays.hashCode(paramTypes);
    }
  }

  public MethodDescriptor getMethodDescriptor(Method method)
  {
    return methodCache.get(method);
  }

  public Object newInstance(long instance)
  {
    ProxyInstance handler = new ProxyInstance(this, instance);
    Object proxy = Proxy.newProxyInstance(cl, interfaces, handler);
    context.getReferenceQueue().registerRef(proxy, instance, cleanup);
    return proxy;
  }

  native MethodHandle getDefaultHandle(Class<?> cls, Method method, Class<?> mhCls);

  @SuppressWarnings("unused")
  private static long unwrapPythonException(Throwable throwable)
  {
    if (throwable == null)
      return 0;
    if (throwable instanceof python.exceptions.PyBaseException)
      return unwrapObject(((python.exceptions.PyBaseException) throwable).get());
    if (throwable instanceof python.lang.PyExc)
      return unwrapObject(throwable);
    return 0;
  }

  private static long unwrapObject(Object obj)
  {
    if (!Proxy.isProxyClass(obj.getClass()))
      return 0;

    try
    {
      InvocationHandler handler = Proxy.getInvocationHandler(obj);
      if (handler instanceof ProxyInstance)
      {
        ProxyInstance jpypeHandler = (ProxyInstance) handler;
        return jpypeHandler.instance;
      }
    } catch (IllegalArgumentException e)
    {
    }
    return 0;
  }

  // exports to JNI
  @SuppressWarnings("unused")
  private static long getInstance(Object obj)
  {
    if (obj == null || !Proxy.isProxyClass(obj.getClass()))
      return 0L;
    InvocationHandler handler = Proxy.getInvocationHandler(obj);
    if (!(handler instanceof ProxyInstance))
      return 0L;
    ProxyInstance proxy = ((ProxyInstance) handler);
    return proxy.instance;
  }

}
