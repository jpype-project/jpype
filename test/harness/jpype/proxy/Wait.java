package jpype.proxy;

public class Wait {
    public static void snooze() throws Throwable {
	    try {
	    System.out.println("Waiting");
	    Thread.sleep(1000);
	    System.out.println("Wake");
	    }
	    catch (Throwable th)
	    {
		    System.out.println("Catch "+th);
		    throw th;
	    }
    }
}
