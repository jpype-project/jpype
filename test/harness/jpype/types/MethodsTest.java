package jpype.types;

public class MethodsTest 
{
	public boolean callBoolean(boolean i) { return i; }
	public byte callByte(byte i) { return i; }
	public char callChar(char i) { return i; }
	public short callShort(short i) { return i; }
	public int callInt(int i) { return i; }
	public long callLong(long i) { return i; }
	public float callFloat(float i) { return i; }
	public double callDouble(double i) { return i; }
	public String callString(String i) { return i; }

	public static Object callStaticObject(Object i) { return i; }
	private static Object callPrivateStaticObject(Object i) { return i; }

	public Object callObject(Object i) { return i; }
	private Object callPrivateObject(Object i) { return i; }
	protected Object callProtectedObject(Object i) { return i; }
}
