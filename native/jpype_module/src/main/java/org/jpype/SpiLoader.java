// --- file: org/jpype/SpiLoader.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package org.jpype;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.JarURLConnection;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.ServiceLoader;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Discovers {@link WrapperService} providers and registers their Python
 * classes and mini-backends with the interpreter at startup.
 */
public final class SpiLoader
{

  private static final Logger LOGGER = Logger.getLogger(SpiLoader.class.getName());

  private SpiLoader()
  {
  }

  /**
   * Discovers every {@link WrapperService} provider via {@code
   * ServiceLoader}, reads each provider's declared {@code .pyspi}
   * resources ({@link WrapperService#getResources()}), and replays them
   * into {@code installer} — eagerly or lazily per each resource's own
   * {@code lazy:} header field. Called once, from
   * {@link MainInterpreter#setInstaller}, at interpreter startup.
   *
   * @param installer the installer to replay discovered resources into.
   */
  public static void load(Installer installer)
  {
    for (WrapperService service : ServiceLoader.load(WrapperService.class))
    {
      for (String resourcePath : service.getResources())
      {
        SpiResource resource = SpiResource.parse(readResource(service.getClass(), resourcePath));
        if (resource.lazy)
        {
          installer.registerLazyClass(resource.module, resource.className, resource.javaInterface, resource.body);
          LOGGER.log(Level.FINE, "SPI lazily registered {0} from {1}",
                  new Object[]
                  {
                    resource.javaInterface, resourcePath
                  });
        }
        else
        {
          if ("backend".equals(resource.kind))
            installer.registerBackend(resource.javaInterface, resource.body);
          else
            installer.registerClass(resource.module, resource.className, resource.javaInterface, resource.body);
          LOGGER.log(Level.FINE, "SPI registered {0} from {1}",
                  new Object[]
                  {
                    resource.javaInterface, resourcePath
                  });
        }
      }
    }
  }

  private static String readResource(Class<?> anchor, String path)
  {
    try (InputStream is = anchor.getResourceAsStream(path))
    {
      if (is == null)
        throw new IllegalStateException(
                "SPI resource not found: " + path + " (relative to " + anchor.getName() + ")");
      return new String(is.readAllBytes(), StandardCharsets.UTF_8);
    } catch (IOException ex)
    {
      throw new IllegalStateException("Failed to read SPI resource: " + path, ex);
    }
  }

  /**
   * Lists every {@code *.pyspi} file directly under an absolute classpath
   * directory (e.g. {@code "/python/io/spi"}), so a {@link WrapperService}
   * can implement {@link WrapperService#getResources()} by pointing at its
   * resource directory once instead of hardcoding each file name. Works
   * whether {@code anchor}'s classes are on the classpath as an exploded
   * directory (e.g. under {@code mvn test}) or packaged inside a jar (e.g.
   * the {@code ant}-built {@code org.jpype.jar}) - both are exercised by
   * this project's own build.
   *
   * @param anchor a class in the same jar/directory as {@code dir}.
   * @param dir absolute classpath directory, e.g. {@code "/python/io/spi"}.
   * @return sorted classpath paths to each {@code .pyspi} file found,
   * empty if {@code dir} doesn't exist.
   */
  public static List<String> listPyspiResources(Class<?> anchor, String dir)
  {
    String prefix = dir.startsWith("/") ? dir.substring(1) : dir;
    if (!prefix.endsWith("/"))
      prefix = prefix + "/";
    URL url = anchor.getResource(dir.startsWith("/") ? dir : ("/" + dir));
    if (url == null)
      return Collections.emptyList();

    List<String> names = new ArrayList<>();
    try
    {
      if ("file".equals(url.getProtocol()))
      {
        File folder = new File(url.toURI());
        String[] files = folder.list((d, name) -> name.endsWith(".pyspi"));
        if (files != null)
          for (String f : files)
            names.add("/" + prefix + f);
      } else if ("jar".equals(url.getProtocol()))
      {
        JarURLConnection conn = (JarURLConnection) url.openConnection();
        conn.setUseCaches(false);
        try (JarFile jar = conn.getJarFile())
        {
          Enumeration<JarEntry> entries = jar.entries();
          while (entries.hasMoreElements())
          {
            String name = entries.nextElement().getName();
            if (name.startsWith(prefix) && name.endsWith(".pyspi")
                    && name.indexOf('/', prefix.length()) < 0)
              names.add("/" + name);
          }
        }
      } else
      {
        throw new IllegalStateException("Unsupported classpath URL protocol for SPI resource scan: " + url);
      }
    } catch (URISyntaxException | IOException ex)
    {
      throw new IllegalStateException("Failed to scan SPI resource directory: " + dir, ex);
    }
    Collections.sort(names);
    return names;
  }

}
