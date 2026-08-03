package jpype.boxing;

public class BoxingTest {
    public static String fnBoxedInteger(Integer arg) {
        return "Integer:" + arg;
    }

    public static String fnBoxedLong(Long arg) {
        return "Long:" + arg;
    }

    public static String fnBoxedShort(Short arg) {
        return "Short:" + arg;
    }

    public static String fnBoxedDouble(Double arg) {
        return "Double:" + arg;
    }

    public static String fnBoxedFloat(Float arg) {
        return "Float:" + arg;
    }

    // Overloaded methods to test that explicit casts still work for disambiguation
    public static String overloaded(Short arg) {
        return "Short:" + arg;
    }

    public static String overloaded(Integer arg) {
        return "Integer:" + arg;
    }

    public static String overloaded(Long arg) {
        return "Long:" + arg;
    }
}
