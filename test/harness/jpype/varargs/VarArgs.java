package jpype.varargs;

class VarArgs
{
	public Object[] rest;

	public static Object[] call(Object ... args)
	{
		return args;
	}

	public static String[] callString(String ... args)
	{
		return args;
	}

	public static int callString0(String str, String ... args)
	{
		return args.length;
	}

	public int callString1(String str, String ... args)
	{
		return args.length;
	}

	public static Integer callOverload(Integer i)
	{
		return i;
	}
	 
	public static String[] callOverload(String str, String ... rest)
	{
		return rest;
	}

	public VarArgs()
	{
	}

	public VarArgs(String s, Object ... rest)
	{
		this.rest=rest;
	}

	public String[] method(String s, String ... rest)
	{
		return rest;
	}
}
