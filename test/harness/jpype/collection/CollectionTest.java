package jpype.collection;

import java.util.Collection;
import java.util.Map;

public class CollectionTest 
{
   public static Object testList(Collection<String> input) {
       System.out.println(input);
       return input;
   }

   public static Object testMap(Map<String, Object> input) {
       System.out.println(input);
       return input;
   }
}
