// --- file: python/collections/package-info.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
/**
 * Java front-end for Python's {@code collections} module, following the
 * same conventions as {@link python.lang} and {@link python.io}.
 *
 * Start from {@link PyCollections#using(python.lang.PyBuiltIn)
 * PyCollections.using(context)} to construct instances of the concrete
 * types in this package, e.g. {@link PyCollections#deque() deque()} for a
 * {@code collections.deque}, or {@link PyCollections#counter() counter()}
 * for a {@code collections.Counter}. Each returned object is a normal Java
 * interface ({@link PyDeque}, {@link PyCounter}, {@link PyOrderedDict},
 * {@link PyDefaultDict}, {@link PyChainMap}) backed by the real Python
 * object, so its methods behave exactly as they would in Python.
 *
 * {@link PyOrderedDict}, {@link PyDefaultDict}, and {@link PyCounter} are
 * all real {@code dict} subclasses in Python, so they extend
 * {@link python.lang.PyDict} here too and reuse its entire {@code Map}
 * surface; only each type's Python-specific extras are added on top.
 * {@link PyChainMap} is a {@code collections.abc.MutableMapping} but
 * <em>not</em> a real {@code dict} subclass, so it extends the weaker
 * {@link python.lang.PyMapping} instead — same reused {@code Map} surface,
 * but an accurate claim about its Python type. {@link PyDeque} is not a
 * {@code list} subclass and does not support slicing, so it stands on its
 * own — a Python-flavored, named subset of {@link java.util.Deque}'s
 * shape.
 */
package python.collections;