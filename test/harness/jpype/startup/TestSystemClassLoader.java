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
}
