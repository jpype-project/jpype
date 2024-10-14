/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype.extension;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.jpype.JPypeContext;
import org.jpype.manager.ClassDescriptor;

/**
 * Internal class for loading extension classes.
 */
public class ExtensionClassLoader extends ClassLoader {

	static final ExtensionClassLoader BUILTIN_LOADER = new ExtensionClassLoader();

	// the java.lang.ref api is USELESS outside of not wanting
	// to keep an object alive when it can be recomputed
	private static final Object cleaner;
	private static final MethodHandle register;

	static {
		Object tmpCleaner = null;
		MethodHandle tmpRegister = null;
		try {
			Class<?> cls = Class.forName("java.lang.ref.Cleaner");
			Method m = cls.getMethod("create");
			tmpCleaner = m.invoke(null);
			m = cls.getMethod("register", Object.class, Runnable.class);
			tmpRegister = MethodHandles.publicLookup().in(cls).unreflect(m);
		} catch (Throwable t) {
			// just leak for the poor souls stuck with a 10 year old JDK
		}
		cleaner = tmpCleaner;
		register = tmpRegister;
	}

	// unfortunately we can't access the one already in ClassLoader
	private final List<Class<?>> classes;
	private final List<Object> cleanables;

	ExtensionClassLoader() {
		super(JPypeContext.getInstance().getClassLoader());
		classes = new ArrayList<>();
		cleanables = new ArrayList<>();
	}

	Class<?> loadClass(String name, byte[] b) {
		Class<?> cls = this.defineClass(name, b, 0, b.length);
		if (this != BUILTIN_LOADER) {
			synchronized (classes) {
				classes.add(cls);
			}
		}
		return cls;
	}

	public boolean isBuiltinLoader() {
		return this == BUILTIN_LOADER;
	}

	public List<Class<?>> getClasses() {
		synchronized (classes) {
			return Collections.unmodifiableList(classes);
		}
	}

	public void register(Class<?> cls, ClassDescriptor desc) throws Throwable {
		if (this == BUILTIN_LOADER || cleaner == null) {
			// don't care, leak away
			return;
		}
		synchronized (cleanables) {
			cleanables.add(register.invoke(cleaner, cls, new Cleaner(desc)));
		}
	}

	/**
	 * This method is only for testing
	 */
	public void closeForTesting() {
		if (this == BUILTIN_LOADER) {
			throw new UnsupportedOperationException();
		}
		/*
		 * While a Class can be collected when its ClassLoader and all of its classes
		 * become unreachable, this did not occur in practice, at least not in testing.
		 * I'm not sure if there is a lingering reference somewhere or if I was just unable
		 * to put enough pressure on the garbage collector to get it to occur. There was
		 * no indication of a leak in VisualVM when looking at heap dump.
		 */
		synchronized (cleanables) {
			if (cleanables.size() > 1) {
				// not in testing
				throw new UnsupportedOperationException();
			}
			for (Object obj : cleanables) {
				try {
					Method m = Class.forName("java.lang.ref.Cleaner$Cleanable").getMethod("clean");
					m.invoke(obj);
				} catch (Throwable t) {
					t.printStackTrace();
				}
			}
		}
	}

	/**
	 * Cleanup all JPClass instances for each Class loaded by this ClassLoader.
	 * </p>
	 * This method is called when the Python Loader associated with this ClassLoader
	 * is collected by the garbage collector. This loader is kept alive via the __loader__
	 * attribute in the Python module where the extension class was defined. Therefore the
	 * module is no longer reachable and we are free to cleanup the JPClasses and remove
	 * the references to our classes so that the JVM's garbage collector can collect them
	 * and this ClassLoader.
	 */
	public void cleanup() {
		if (this == BUILTIN_LOADER) {
			throw new UnsupportedOperationException();
		}
		JPypeContext.getInstance().getTypeManager().destroy(this);
		classes.clear();
	}

	private static native void delete(long type);

	static native void clearHost(long type);

	private static class Cleaner implements Runnable {

		private final long jclass;

		private Cleaner(ClassDescriptor desc) {
			jclass = desc.classPtr;
			desc.cls = null;
		}

		@Override
		public void run() {
			delete(jclass);
		}
	}
}
