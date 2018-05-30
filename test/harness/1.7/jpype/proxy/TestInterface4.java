package jpype.proxy;

import java.util.List;

public interface TestInterface4 {

    ReturnObject testMethodObject();

    int testMethodInt();

    String testMethodString();

    List<ReturnObject> testMethodList(int noOfValues);
}
