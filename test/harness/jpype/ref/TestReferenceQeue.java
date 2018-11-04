package jpype.ref;

public class TestReferenceQeue
{
  public static void main(String[] args)
  {
//    final JPypeReferenceQueue queue = new JPypeReferenceQueue();
//    
//    new Thread(new Runnable() {
//
//      public void run()
//      {
//        queue.startManaging();
//      }
//      
//    }).start();
//    
//    Object dummy = new Object();
//    long dummyAddress = 123456;
//    
//    JPypeReference ref = new JPypeReference(dummy, queue);
//    
//    queue.registerRef(ref, dummyAddress);
//    
//    System.out.println("ref is enqueued? "+ref.isEnqueued());
//    
//    long start = System.currentTimeMillis();
//    dummy = null;
//    while (System.currentTimeMillis()-start < 30000 && ! (ref.isEnqueued()))
//    {
//      System.gc();
//      System.out.print(".");
//      try
//      {
//        Thread.sleep(250);
//      }
//      catch(InterruptedException ex) {}
//    }
//    
//    System.out.println();
//    System.out.println("ref is enqueued? "+ref.isEnqueued());
//    queue.stop();
  }
}
