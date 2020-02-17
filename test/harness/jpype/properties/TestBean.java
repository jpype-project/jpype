package jpype.properties;

public class TestBean {

	private String getA;

	private String setA;

	private String property1;

	private String property2Invisible;

	private String property3;

	public String property4;

	private String property5;

	private String property6;

	public String property7;

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


	public static String m1;
	public String m2;
	public String m3;
	public String m4;
	public String m5;

	public String getPropertyMember() {
		return this.m2;
	}

	public void setPropertyMember(String value) {
		this.m2 = value;
	}

	public static String getPropertyStatic() {
		return m1;
	}

	public static void setPropertyStatic(String value) {
		m1 = value;
	}

	public String getReadOnly() {
		return this.m3;
	}

	public void setWriteOnly(String value) {
		this.m4 = value;
	}

	public void setWith(String value) {
		this.m5 = value;
	}

	public String getWith() {
		return this.m5;
	}

	public void setFailure1(String value, int i) {
	}

	public String getFailure2(int i) {
		return "fail";
	}


}
