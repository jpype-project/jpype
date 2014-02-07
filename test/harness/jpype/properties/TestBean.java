package jpype.properties;

public class TestBean {
	private String property1;

	private String property2Invisible;

	private String property3;

	private String property4;

	private String property5;

	private String property6;

	private String property7;

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

	public String getProperty6() {
		return "get" + property7;
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

	public String returnProperty5() {
		return "return" + this.property5;
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

	public void setProperty5(String property5) {
		this.property5 = "set" + property5;
	}
	
	public void setProperty6(String property6) {
		this.property7 = "set" + property6;
	}
}
