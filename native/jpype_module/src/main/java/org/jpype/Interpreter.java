// --- file: org/jpype/Interpreter.java ---
package org.jpype;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.Writer;
import python.lang.PyBuiltIn;
import python.lang.PyDict;

/**
 * Defines the core execution capabilities of a Python interpreter instance.
 */
public interface Interpreter extends AutoCloseable
{
   PyBuiltIn getBuiltIn();

    /**
     * Redirects this interpreter's {@code sys.stdout} to a Java stream.
     *
     * The stream is exposed to Python via its {@code toPython()} method
     * (added to every {@code java.io} stream by JPype's IO customizers), so
     * this works for any {@link Writer}/{@link OutputStream} subclass,
     * including {@link java.io.PrintStream}.
     *
     * @param out the stream to receive everything written to Python's stdout.
     */
    default void setOutput(OutputStream out) { installStdio("stdout", out); }

    /**
     * @see #setOutput(OutputStream)
     */
    default void setOutput(Writer out) { installStdio("stdout", out); }

    /**
     * Redirects this interpreter's {@code sys.stderr} to a Java stream.
     *
     * @param err the stream to receive everything written to Python's stderr.
     * @see #setOutput(OutputStream)
     */
    default void setError(OutputStream err) { installStdio("stderr", err); }

    /**
     * @see #setError(OutputStream)
     */
    default void setError(Writer err) { installStdio("stderr", err); }

    /**
     * Redirects this interpreter's {@code sys.stdin} to a Java stream.
     *
     * @param in the stream Python reads from when it reads its stdin.
     * @see #setOutput(OutputStream)
     */
    default void setInput(InputStream in) { installStdio("stdin", in); }

    /**
     * @see #setInput(InputStream)
     */
    default void setInput(Reader in) { installStdio("stdin", in); }

    /**
     * Restores {@code sys.stdout} to Python's original stream.
     */
    default void resetOutput() { execInScratchScope("import sys; sys.stdout = sys.__stdout__"); }

    /**
     * Restores {@code sys.stderr} to Python's original stream.
     */
    default void resetError() { execInScratchScope("import sys; sys.stderr = sys.__stderr__"); }

    /**
     * Restores {@code sys.stdin} to Python's original stream.
     */
    default void resetInput() { execInScratchScope("import sys; sys.stdin = sys.__stdin__"); }

    /**
     * Binds {@code stream} into a scratch namespace and assigns
     * {@code sys.<name> = stream.toPython()} in one call.
     */
    private void installStdio(String name, Object stream)
    {
        PyBuiltIn b = getBuiltIn();
        PyDict scope = b.dictFromMap(java.util.Collections.singletonMap("_stream", stream));
        b.exec("import sys; sys." + name + " = _stream.toPython()", scope, scope);
    }

    /**
     * Runs a statement that needs no bound variables in a fresh, empty
     * scope - {@code exec()}'s globals/locals must be a real dict, so a
     * bare Java {@code null} is not a substitute for Python's own
     * "omitted" default.
     */
    private void execInScratchScope(String statement)
    {
        PyBuiltIn b = getBuiltIn();
        PyDict scope = b.getBackend().newDict();
        b.exec(statement, scope, scope);
    }

    /**
     * Closes the interpreter instance and disposes of its associated resources.
     */
    @Override
    void close();
}