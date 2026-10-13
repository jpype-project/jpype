package jpype.boxing;

public class EdgeCaseTest {
    // Test that Object still prefers exact matches over boxed conversions
    public static String acceptObject(Object arg) {
        return "Object:" + arg.getClass().getSimpleName();
    }

    // Test mixed primitive and boxed overloads
    public static String mixed(int arg) {
        return "int:" + arg;
    }

    public static String mixed(Integer arg) {
        return "Integer:" + arg;
    }

    // Test Object vs boxed type - Object should not win over boxed with implicit conversion
    public static String objectVsBoxed(Object arg) {
        return "Object";
    }

    public static String objectVsBoxed(Integer arg) {
        return "Integer";
    }

    // Test widening conversions with boxed types
    public static String widen(Long arg) {
        return "Long:" + arg;
    }

    // Ambiguous overload that should still require explicit cast
    public static String ambiguous(Integer arg) {
        return "Integer:" + arg;
    }

    public static String ambiguous(Long arg) {
        return "Long:" + arg;
    }
}
