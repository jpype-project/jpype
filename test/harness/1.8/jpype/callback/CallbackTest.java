package jpype.callback;

class CallbackTest
{
	public interface Callable
	{
		int execute();
	}

	public CallbackTest()
	{
	}

	public Callable getAnonymous()
	{
		return new Callable()
		{
			public int execute()
			{
				return 1;
			}
		};
	}

	public Callable getLambda()
	{
		return () -> 2;
	}
}
