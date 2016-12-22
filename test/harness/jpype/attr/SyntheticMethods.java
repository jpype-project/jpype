package jpype.attr;

import java.util.List;

public class SyntheticMethods {
    public static interface GenericInterface<T> {
        void foo(T value);
    }

    public static class GenericImpl implements GenericInterface<List> {
        public List mListValue = null;
        public void foo(List value) {
            mListValue = value;
        }
    }

}