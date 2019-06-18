package jpype.str;

public class Test
{

	public static String staticField = "staticField";
	public String memberField = "memberField";

	public static String staticCall()
	{
		return "staticCall";
	}

	public String memberCall()
	{
		return "memberCall";
	}

	public static final String array[] =
	{"apples","banana","cherries","dates","elderberry"};

	public static String callProxy(StringFunction f, String s)
	{
		return f.call(s);
	}
}


