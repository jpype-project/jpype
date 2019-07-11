package jpype.bug;

import jdk.internal.reflect.CallerSensitive;

public class Caller
{
    @CallerSensitive
    public static Class callStatic()
    {
	return null;
    }

    @CallerSensitive
    public Class callMember()
    {
	return null;
    }

    @CallerSensitive
    public Class call(Object a, Object b)
    {
	return null;
    }
 
};
