package jpype.extension;

public abstract class TestBase {

	public TestBase() {
	}

	public TestBase(int i) {
	}

	public TestBase(Object o) {
	}

	public int identity(int i) {
		return i;
	}

	public Object identity(Object o) {
		return o;
	}
}
