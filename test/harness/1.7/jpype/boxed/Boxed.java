package jpype.boxed;

public class Boxed
{ 
  // Call each type
  static public boolean callBoolean(boolean i) { return i; }
  static public byte    callByte(byte i)       { return i; }
  static public char    callChar(char i)       { return i; }
  static public short   callShort(short i)     { return i; }
  static public int     callInteger(int i)     { return i; }
  static public long    callLong(long i)       { return i; }
  static public float   callFloat(float i)     { return i; }
  static public double  callDouble(double i)   { return i; }

  // Create a boxed type
  static public Short newShort(short i) { return i; }
  static public Integer newInteger(int i) { return i; }
  static public Long newLong(long i) { return i; }
  static public Float newFloat(float i) { return i; }
  static public Double newDouble(double i) { return i; }

  // Check which is called
  static public int whichShort(short i) { return 1; }
  static public int whichShort(Short i) { return 2; }

  static public int whichInteger(int i) { return 1; }
  static public int whichInteger(Integer i) { return 2; }

  static public int whichLong(long i) { return 1; }
  static public int whichLong(Long i) { return 2; }

  static public int whichFloat(float i) { return 1; }
  static public int whichFloat(Float i) { return 2; }

  static public int whichDouble(double i) { return 1; }
  static public int whichDouble(Double i) { return 2; }
}

