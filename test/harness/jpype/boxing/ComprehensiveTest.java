package jpype.boxing;

import java.util.List;
import java.util.ArrayList;

public class ComprehensiveTest {
    // Test null handling
    public static String acceptInteger(Integer arg) {
        return arg == null ? "null" : "Integer:" + arg;
    }

    // Test Boolean (non-numeric boxed type)
    public static String acceptBoolean(Boolean arg) {
        return "Boolean:" + arg;
    }

    // Test collections with boxed types
    public static String acceptList(List<Integer> list) {
        return "List<Integer>:size=" + list.size();
    }

    // Test varargs with boxed types
    public static String varargs(Integer... args) {
        return "varargs:count=" + args.length;
    }

    // Test return values
    public static Integer returnBoxed(int val) {
        return val;
    }

    // Test Number hierarchy (Integer extends Number)
    public static String acceptNumber(Number arg) {
        return "Number:" + arg.getClass().getSimpleName();
    }

    // Test Comparable interface (Integer implements Comparable)
    public static String acceptComparable(Comparable<Integer> arg) {
        return "Comparable";
    }
}
