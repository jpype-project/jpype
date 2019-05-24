package jpype.reflect;

public class ReflectionTest
{
	public String publicField = "public";
	private String privateField ="private";

	public ReflectionTest()
	{
	}

	public String publicMethod()
	{
		return "public";
	}

	private String privateMethod()
	{
		return "private";
	}

	@Annotation("annotation")
	public void annotatedMethod()
	{
	}
}

