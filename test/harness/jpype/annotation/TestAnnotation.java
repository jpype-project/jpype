package jpype.annotation;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Test case for issue #880: Annotation methods must use virtual JNI calls
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface TestAnnotation {
	String value() default "default value";
	int count() default 42;
}
