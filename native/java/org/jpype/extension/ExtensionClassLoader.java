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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.jpype.JPypeContext;

/**
 * Internal class for loading extension classes.
 */
public class ExtensionClassLoader extends ClassLoader {

	static final ExtensionClassLoader BUILTIN_LOADER = new ExtensionClassLoader();

	// unfortunately we can't access the one already in ClassLoader
	private final List<Class<?>> classes;

	ExtensionClassLoader() {
		super(JPypeContext.getInstance().getClassLoader());
		classes = new ArrayList<>();
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
		synchronized (classes) {
			JPypeContext.getInstance().getTypeManager().destroy(this);
			classes.clear();
		}
	}

	static native void clearHost(long type);
}
