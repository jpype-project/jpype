package org.jpype.extension;

import java.lang.reflect.Method;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public final class AnnotationDecl {
	public final Class<?> cls;
	private final Map<String, Object> elements;

	public AnnotationDecl(Class<?> cls) {
		this.cls = cls;
		Method[] methods = cls.getDeclaredMethods();
		this.elements = new HashMap<>(methods.length);
		for (Method m : methods) {
			elements.put(m.getName(), m.getDefaultValue());
		}
	}

	public Map<String, Object> getElements() {
		return Collections.unmodifiableMap(elements);
	}

	public boolean contains(String key) {
		return elements.containsKey(key);
	}

	public void addElements(Map<String, Object> elements) {
		for (String key : elements.keySet()) {
			if (!this.elements.containsKey(key)) {
				throw new UnsupportedOperationException(key);
			}
		}
		this.elements.putAll(elements);
		validate();
	}

	private void validate() {
		for (Map.Entry<String, Object> entry : elements.entrySet()) {
			if (entry.getValue() == null) {
				// we will take the missing name from the msg and raise a KeyError
				throw new IllegalArgumentException(entry.getKey());
			}
		}
	}
}
