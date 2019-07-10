package jpype.bug;

import jdk.internal.reflect.CallerSensitive;
import jdk.internal.reflect.Reflection;

public class Caller
{
    @CallerSensitive
    public static Class callStatic()
    {
	return Reflection.getCallerClass();
    }

    @CallerSensitive
    public Class callMember()
    {
	return Reflection.getCallerClass();
    }

    @CallerSensitive
    public Class call(Object a, Object b)
    {
	return Reflection.getCallerClass();
    }
 
};
