package jpype.exc;

public class WierdException extends Exception
{

	public WierdException(int i, float f, Object o)
	{
		super("Got it");
	}

	public static void testThrow() throws WierdException
	{
		throw new WierdException(1,2,new Object());
	}

}
