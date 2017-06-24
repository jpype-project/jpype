package jpype.closeable;

class AutoClose implements java.io.Closeable
{
	public static boolean closed=false;
	public static String printed="";

	public AutoClose()
	{
	}

	public void print(String value)
	{
		printed=value;
	}

	public void close() throws java.io.IOException
	{
		closed=true;
	}

}
