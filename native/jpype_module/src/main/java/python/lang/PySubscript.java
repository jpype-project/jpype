// --- file: python/lang/PySubscript.java ---
package python.lang;

/**
 * Represents objects that can be used as subscripts in Python-style indexing
 * operations.
 *
 * <p>
 * This interface defines the contract for objects that may appear inside
 * subscription expressions such as {@code obj[item]}. This includes integer
 * indices, slice objects, and other subscript forms supported by Python-like
 * operations.
 *
 * <p>
 * Examples of objects that can implement this interface include:
 * <ul>
 * <li>Integer indices for accessing single elements</li>
 * <li>Slice objects for accessing ranges of elements</li>
 * <li>Custom subscript types for advanced indexing behavior</li>
 * </ul>
 *
 * <p>
 * Implementations should be compatible with Python subscription semantics where
 * applicable.
 *
 * @see PySlice
 */
public interface PySubscript extends PyObject
{
}
