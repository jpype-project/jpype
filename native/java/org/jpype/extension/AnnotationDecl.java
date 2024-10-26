package org.jpype.extension;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

import org.jpype.asm.Type;

public final class AnnotationDecl {
	public final Class<?> cls;
	final Map<String, ValueHelper> elements;

	// FIXME: is declaration order of annotations important?

	public AnnotationDecl(Class<?> cls) {
		this.cls = cls;
		Method[] methods = cls.getDeclaredMethods();
		this.elements = new HashMap<>(methods.length);
		for (Method m : methods) {
			elements.put(m.getName(), new ValueHelper(m));
		}
	}

	public void addElements(Map<String, Object> elements) {
		for (Map.Entry<String, Object> entry : elements.entrySet()) {
			String key = entry.getKey();
			ValueHelper helper = this.elements.get(key);
			if (helper == null) {
				throw new UnsupportedOperationException(key);
			}
			helper.setValue(entry.getValue());
		}
		validate();
	}

	private void validate() {
		for (Map.Entry<String, ValueHelper> entry : elements.entrySet()) {
			if (entry.getValue().isEmpty()) {
				// we will take the missing name from the msg and raise a KeyError
				throw new IllegalArgumentException(entry.getKey());
			}
		}
	}

	String getDescriptor() {
		return Type.getDescriptor(cls);
	}

	static final class ValueHelper {
		// this is necessary because asm will write a Long value into an int
		// it won't cause an exception until something tries to get the annotation value

		final Class<?> type;
		Object value;

		ValueHelper(Method method) {
			this.type = method.getReturnType();
			this.value = method.getDefaultValue();
		}

		private boolean isEmpty() {
			return value == null;
		}

		private static Byte toByte(Number value) {
			if (value.longValue() == value.byteValue()) {
				return value.byteValue();
			}
			throw new IllegalArgumentException("Value: "+value+" doesn't fit in a byte");
		}

		private static Short toShort(Number value) {
			if (value.longValue() == value.shortValue()) {
				return value.shortValue();
			}
			throw new IllegalArgumentException("Value: "+value+" doesn't fit in a short");
		}

		private static Integer toInteger(Number value) {
			if (value.longValue() == value.intValue()) {
				return value.intValue();
			}
			throw new IllegalArgumentException("Value: "+value+" doesn't fit in a integer");
		}

		private static Float toFloat(Number value) {
			if (value.doubleValue() == value.floatValue()) {
				return value.floatValue();
			}
			throw new IllegalArgumentException("Value: "+value+" doesn't fit in a double");
		}

		private void setValue(Object value) {
			if (!type.isPrimitive() || type == Boolean.TYPE) {
				// bool doesn't suffer from box conversion trouble
				this.value = value;
				return;
			}
			if (type == Byte.TYPE) {
				this.value = toByte((Number)value);
			} else if (type == Character.TYPE) {
				this.value = (Character) value;
			} else if (type == Short.TYPE) {
				this.value = toShort((Number)value);
			} else if (type == Integer.TYPE) {
				this.value = toInteger((Number)value);
			} else if (type == Long.TYPE) {
				this.value = (Long) value;
			} else if (type == Float.TYPE) {
				this.value = toFloat((Number)value);
			} else if (type == Double.TYPE) {
				this.value = (Double) value;
			}
		}
	}
}
