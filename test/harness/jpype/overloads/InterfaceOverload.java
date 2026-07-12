package jpype.overloads;

/**
 * Test case for issue #844: Overloaded methods across ancestor types
 * are sometimes not detected.
 */
public class InterfaceOverload {

	public interface DoStuffNoArgs {
		default void doStuff() {
		}
	}

	public interface DoStuffWithArgs {
		default void doStuff(String arg) {
		}
	}

	/**
	 * Class implementing both interfaces - should have both doStuff overloads available
	 */
	public static class BothOverloads implements DoStuffWithArgs, DoStuffNoArgs {
	}

	/**
	 * Helper to track which overload was called
	 */
	public static class CallTracker implements DoStuffWithArgs, DoStuffNoArgs {
		public String lastCall = null;

		@Override
		public void doStuff() {
			lastCall = "noargs";
		}

		@Override
		public void doStuff(String arg) {
			lastCall = "string:" + arg;
		}
	}
}
