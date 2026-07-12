// --- file: org/jpype/WrapperService.java ---
package org.jpype;

import java.util.Collections;

/**
 * Service provider interface for extending the Java view of Python types.
 * Implement this to expose your own package's Python types as Java
 * interfaces, then register the implementation via Jigsaw:
 * <pre>{@code
 * provides org.jpype.WrapperService with my.package.NumpyWrapperService;
 * }</pre>
 * {@code python.io.PyIOWrapperService} is a complete worked example.
 */
public interface WrapperService {

    /**
     * The fully qualified Python module names this service provides
     * bindings for, e.g. {@code {"numpy"}}. A single provider may cover
     * more than one module name, e.g. a public facade module and the
     * internal C-accelerator module backing it.
     */
    String[] getModuleNames();

    /**
     * A version string for this provider's bindings, e.g. the version of
     * the Python package they target.
     *
     * @return a version string.
     */
    String getVersion();

    /**
     * Optional: Provides specialized logic for the backend to handle
     * this specific type (e.g., custom memory mapping for buffers).
     */
    default void initialize(Backend backend) {
        // Default: no specialized backend initialization
    }

    /**
     * Classpath paths (resolved relative to this service's own class, e.g.
     * via {@code getClass().getResourceAsStream(path)}) to every one of
     * this provider's {@code .pyspi} resources — one per Python class it
     * registers, plus one per mini-backend it needs. Read by
     * {@code SpiLoader} at startup and replayed into {@code Installer}.
     *
     * A typical implementation scans its own resource directory rather
     * than hardcoding each path, e.g.:
     * <pre>{@code
     * return SpiLoader.listPyspiResources(MyWrapperService.class, "/my/package/spi");
     * }</pre>
     *
     * <h3>{@code .pyspi} file format</h3>
     * A small {@code key: value} header, a line containing only
     * {@code ---}, then a blob of Python source that (when {@code exec}'d)
     * binds a top-level name {@code METHODS} to a {@code dict[str,
     * Callable]} mapping method names to implementations.
     *
     * <p>Class registration, eager — replayed immediately at startup:
     * <pre>
     * kind: class
     * module: _io
     * class: BytesIO
     * interface: python.io.PyBytesIO
     * ---
     * METHODS = {
     *     "getvalue": lambda x: x.getvalue(),
     * }
     * </pre>
     *
     * <p>Class registration, lazy — only imported/registered the first
     * time an instance of the class is actually seen crossing into Java:
     * <pre>
     * kind: class
     * module: _io
     * class: StringIO
     * interface: python.io.PyStringIO
     * lazy: true
     * ---
     * METHODS = {
     *     "getvalue": lambda x: x.getvalue(),
     * }
     * </pre>
     *
     * <p>Mini-backend registration — always eager, for a small
     * provider-owned interface (like {@code python.io.IO}) that needs its
     * own dispatch dict independent of the shared {@link Backend}:
     * <pre>
     * kind: backend
     * interface: python.io.IO
     * ---
     * METHODS = {
     *     "bytesIO": lambda: __import__("io").BytesIO(),
     * }
     * </pre>
     *
     * Default empty.
     */
    default Iterable<String> getResources() {
        return Collections.emptyList();
    }
}