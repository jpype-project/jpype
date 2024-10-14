package jpype.extension;

public class TestDanglingObject {

	private final Object ref;

	public TestDanglingObject(Object ref) {
		this.ref = ref;
	}

	public String test() {
		return ref.toString();
	}
}
