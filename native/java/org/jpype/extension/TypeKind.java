package org.jpype.extension;

import org.jpype.asm.Opcodes;

enum TypeKind {
	BOOL(Opcodes.ILOAD, "java/lang/Boolean"),
	BYTE(Opcodes.ILOAD, "java/lang/Byte"),
	CHAR(Opcodes.ILOAD, "java/lang/Character"),
	SHORT(Opcodes.ILOAD, "java/lang/Short"),
	INT(Opcodes.ILOAD, "java/lang/Integer"),
	LONG(Opcodes.LLOAD, "java/lang/Long"),
	FLOAT(Opcodes.FLOAD, "java/lang/Float"),
	DOUBLE(Opcodes.DLOAD, "java/lang/Double"),
	VOID(-1, "java/lang/Void"),
	OBJECT(Opcodes.ALOAD, null);

	final int load;
	final String boxedClass;

	TypeKind(int load, String boxedClass) {
		this.load = load;
		this.boxedClass = boxedClass;
	}

	static TypeKind of(Class<?> type) {
		if (type == null || type == Void.TYPE) {
			return VOID;
		}
		if (!type.isPrimitive()) {
			return OBJECT;
		}
		if (type == Boolean.TYPE) {
			return BOOL;
		}
		if (type == Byte.TYPE) {
			return BYTE;
		}
		if (type == Character.TYPE) {
			return CHAR;
		}
		if (type == Short.TYPE) {
			return SHORT;
		}
		if (type == Integer.TYPE) {
			return INT;
		}
		if (type == Long.TYPE) {
			return LONG;
		}
		if (type == Float.TYPE) {
			return FLOAT;
		}
		if (type == Double.TYPE) {
			return DOUBLE;
		}
		throw new IllegalArgumentException("impossible type " + type.toString());
	}
}
