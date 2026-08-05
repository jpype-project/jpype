package jpype.annotation;

/**
 * A class with annotations for testing
 */
@TestAnnotation(value = "test", count = 100)
public class AnnotatedClass {
	public String getName() {
		return "AnnotatedClass";
	}
}
