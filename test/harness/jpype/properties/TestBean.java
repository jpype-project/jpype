package jpype.properties;

public class TestBean {

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
