package jpype.types;

public interface Outer
{
  Object get();

  public class Inner implements Outer
  {
	  public Object get()
	  {
		  return null;
	  }
  }

}
