package org.jpype.fail;

// To test this with more than one path, we need a second
// copy.  Java caches the result of the first attempt
public class BadInitializer2
{
    static
    {
        int[] i = new int[0];
        i[1] = 2;
    }
}
