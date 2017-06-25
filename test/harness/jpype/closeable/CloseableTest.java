package jpype.closeable;

class CloseableTest implements java.io.Closeable
{
  public static boolean closed=false;
  public static String printed="";
  public static boolean willfail=false;
  public static boolean failed=false;

  public static void reset()
  {
    closed = false;
    willfail = false;
    failed = false;
    printed = "";
  }

  public CloseableTest()
  {
  }

  public void print(String value)
  {
    printed = value;
  }

  public void throwException()
  {
    throw new RuntimeException("oh no!");
  }

  public void close() throws java.io.IOException
  {
    closed = true;
    if (willfail)
    {
      failed = true;
      throw new java.io.IOException("oh my?");
    }
  }

}
