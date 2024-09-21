package org.jpype.extension;

final class Parameter {
	final Class<?> type;
	final TypeKind kind;
	final int slot;

	Parameter(Class<?> type, int slot) {
		this.type = type;
		this.kind = TypeKind.of(type);
		this.slot = slot;
	}

	int getNextSlot() {
		if (kind == TypeKind.LONG || kind == TypeKind.DOUBLE) {
			return slot + 2;
		}
		return slot + 1;
	}
}
