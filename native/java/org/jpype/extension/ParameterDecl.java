package org.jpype.extension;

final class ParameterDecl extends JavaDecl {
	final Class<?> type;
	final TypeKind kind;
	final int slot;
	final String name;
	long id;

	ParameterDecl(Class<?> type, String name, int slot) {
		this.type = type;
		this.kind = TypeKind.of(type);
		this.slot = slot;
		this.name = name;
	}

	int getNextSlot() {
		if (kind == TypeKind.LONG || kind == TypeKind.DOUBLE) {
			return slot + 2;
		}
		return slot + 1;
	}
}
