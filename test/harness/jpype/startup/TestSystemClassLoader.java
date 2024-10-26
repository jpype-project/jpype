/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package jpype.startup;

import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Paths;

public class TestSystemClassLoader extends URLClassLoader {

	public TestSystemClassLoader(ClassLoader parent) throws Throwable {
		super(new URL[0], parent);
	}

	public void addPath(String path) throws Throwable {
		addURL(Paths.get(path).toAbsolutePath().toUri().toURL());
	}

	public void addPaths(String[] paths) throws Throwable {
		for (String path : paths) {
			addPath(path);
		}
	}

	@SuppressWarnings("unused") // needed to start with agent
	private void appendToClassPathForInstrumentation(String path) throws Throwable {
		addPath(path);
	}
}
