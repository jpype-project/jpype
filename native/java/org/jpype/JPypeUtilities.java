package org.jpype;

import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;

public class JPypeUtilities
{

  public static Path getJarPath(Class c)
  {
    try
    {
      return Paths.get(c.getProtectionDomain().getCodeSource().getLocation()
              .toURI()).getParent();
    } catch (URISyntaxException ex)
    {
      return null;
    }
  }

}
