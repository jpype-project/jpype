package jpype.values;

public class FieldsTest 
{
	// Tests for access
  private static Object staticPrivateObjectField
		= new String("private static object field");
  private Object privateObjectField
		= new String("private object field");
  public static Object staticObjectField;
  public final static Object finalStaticObjectField
		= new String("final static object field");
  public final Object finalObjectField
		= new String("final object field");
  
	// Tests for type conversion
  public boolean booleanField;
  public char charField;
  public short shortField;
  public long longField;
  public int intField;
  public float floatField;
  public double doubleField;
  public Object objectField;
}
