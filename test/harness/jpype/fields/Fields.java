package jpype.fields;

public class Fields {

	public static boolean static_bool;
	public static char    static_char;
	public static byte    static_byte;
	public static short   static_short;
	public static int     static_int;
	public static long    static_long;
	public static float   static_float;
	public static double  static_double;
	public static Object  static_object;

	public boolean member_bool;
	public char    member_char;
	public byte    member_byte;
	public short   member_short;
	public int     member_int;
	public long    member_long;
	public float   member_float;
	public double  member_double;
	public Object  member_object;

	public static boolean getStaticBool()   { return static_bool; }
	public static char    getStaticChar()   { return static_char; }
	public static byte    getStaticByte()   { return static_byte; }
	public static short   getStaticShort()  { return static_short; }
	public static int     getStaticInt()    { return static_int; }
	public static long    getStaticLong()   { return static_long; }
	public static float   getStaticFloat()  { return static_float; }
	public static double  getStaticDouble() { return static_double; }
	public static Object  getStaticObject() { return static_object; }

	public boolean getMemberBool()   { return member_bool; }
	public char    getMemberChar()   { return member_char; }
	public byte    getMemberByte()   { return member_byte; }
	public short   getMemberShort()  { return member_short; }
	public int     getMemberInt()    { return member_int; }
	public long    getMemberLong()   { return member_long; }
	public float   getMemberFloat()  { return member_float; }
	public double  getMemberDouble() { return member_double; }
	public Object  getMemberObject() { return member_object; }
}
