package jpype.extension;

public abstract class TestBase {

	public int initCount = 0;
	protected int index = 0;

	public TestBase() {
		initCount++;
	}

	public TestBase(int i) {
		initCount++;
	}

	public TestBase(Object o) {
		initCount++;
	}

	public int identity(int i) {
		return i;
	}

	public Object identity(Object o) {
		return o;
	}
}
