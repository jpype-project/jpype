package jpype.method;

import jdk.internal.reflect.CallerSensitive;

/** Caller sensitive methods need special handling.
 *
 * So we need to test each possible path
 * (different return types and arguments).
 *
 * We will pretend our methods are also CallerSensitive.
 */
public class Caller
{
    @CallerSensitive
    public static Object callObjectStatic()
    {
	return new Caller();
    }

    @CallerSensitive
    public Object callObjectMember()
    {
	return new Caller();
    }

    @CallerSensitive
    public static void callVoidStatic()
    {
    }

    @CallerSensitive
    public void callVoidMember()
    {
    }

    @CallerSensitive
    public static int callIntegerStatic()
    {
	return 123;
    }

    @CallerSensitive
    public int callIntegerMember()
    {
	return 123;
    }

    @CallerSensitive
    public Object callArgs(Object a, Object b)
    {
	return b;
    }

    @CallerSensitive
    public int callArg1(int a)
    {
	return a;
    }

    @CallerSensitive
    public Object callVarArgs(Object a, Object... b)
    {
	return b;
    }
 
};
