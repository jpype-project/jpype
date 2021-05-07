package org.jpype.unsatisfied;

public class TestClass
{

  public static void main(String[] args)
  {
    System.out.println("Hi Freaks");
  }

  public static UnsatisfiedClass methodWithUnsatisfiedReturnType()
  {
    return null;
  }
}

class UnsatisfiedClass
{
}
