package jpype.properties;

public class TestBean {
	private String property1;

	private String property2Invisible;

	private String property3;

	private String property4;

	public String getProperty1() {
		return "get" + property1;
	}

	public String getProperty2() {
		return "get" + property2Invisible;
	}

	public String getProperty3() {
		return "get" + property3;
	}

	public String abcProperty4() {
		return "abc" + property4;
	}

	public String property1() {
		return "method";
	}

	protected String property2() {
		return "method";
	}

	protected String property3() {
		return "method";
	}

	public void setProperty1(String property1) {
		this.property1 = "set" + property1;
	}

	public void setProperty2(String property2) {
		this.property2Invisible = "set" + property2;
	}

	public void setProperty3(String property3) {
		this.property3 = "set" + property3;
	}
}
