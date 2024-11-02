package jpype.extension;

public class TestDanglingObject {

	private static TestDanglingObject instance;
	private final Object ref;

	public TestDanglingObject(Object ref) {
		this.ref = ref;
		instance = this;
	}

	public static String test() {
		return instance.ref.toString();
	}

	@SuppressWarnings("deprecation")
	public static Object newInstance() throws Throwable {
		return instance.ref.getClass().newInstance();
	}
}
