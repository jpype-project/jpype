// --- file: org/jpype/bridge/WrapperProvider.java ---
package org.jpype.bridge;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * The central registry and router for Python-to-Java type extensions. It
 * discovers WrapperServices via Jigsaw and caches mapping for performance.
 */
public final class WrapperProvider
{

  private final Map<String, List<WrapperService>> moduleToServiceMap = new HashMap<>();
  private final Map<String, Class<?>[]> typeCache = new ConcurrentHashMap<>();

  public WrapperProvider()
  {
    // 1. Discover all services via Jigsaw ServiceLoader
    ServiceLoader<WrapperService> loader = ServiceLoader.load(WrapperService.class);
    for (WrapperService service : loader)
    {
      for (String module : service.getModuleNames())
        moduleToServiceMap.computeIfAbsent(module, k -> new ArrayList<>()).add(service);
    }
  }

  /**
   * Forks the request to the appropriate service based on the class name.
   *
   * * @param clsName Fully qualified Python name (e.g., "numpy.ndarray")
   * @return Aggregated array of interfaces from all matching services.
   */
  public Class<?>[] getInterfaces(String clsName)
  {
    return typeCache.computeIfAbsent(clsName, this::lookup);
  }

  private Class<?>[] lookup(String name)
  {
    Set<Class<?>> aggregated = new LinkedHashSet<>();

    // Extract the module part (everything before the last dot)
    int lastDot = name.lastIndexOf('.');
    if (lastDot == -1)
      return new Class<?>[0];
    String moduleName = name.substring(0, lastDot);

    // Find the service responsible for this module
    List<WrapperService> services = moduleToServiceMap.get(moduleName);
    if (services != null)
    {
      for (WrapperService service : services)
      {
        Class<?>[] interfaces = service.getInterfaces(name);
        if (interfaces != null)
          aggregated.addAll(Arrays.asList(interfaces));
      }
    }

    return aggregated.toArray(new Class<?>[0]);
  }
}
