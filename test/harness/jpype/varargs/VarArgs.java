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
