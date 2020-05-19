package org.jpype.fail;

public class BadInitializer
{
    static
    {
        int[] i = new int[0];
        i[1] = 2;
    }
}
