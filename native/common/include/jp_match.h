/*****************************************************************************
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
 *****************************************************************************/
#ifndef JP_MATCH_H
#define JP_MATCH_H

class JPConversion;

class JPMatch
{
public:

	enum Type
	{
		_none = 0,
		_explicit = 1,
		_implicit = 2,
		_derived = 3,
		_exact = 4
	} ;

public:
	JPMatch();
	JPMatch(JPJavaFrame *frame, PyObject *object);

	/**
	 * Get the JPClass associated with the Python object, if any.
	 *
	 * Cached alongside getJValue() -- both are resolved together on first
	 * use since some JPConversion::matches() implementations (e.g.
	 * JPConversionUnbox) read the cached jvalue directly, relying on an
	 * earlier matches() call on the same argument having already resolved
	 * it.
	 *
	 * @return the class, or nullptr if not a Java value.
	 */
	JPClass *getJPClass();

	/**
	 * Get the jvalue associated with the Python object.
	 *
	 * Only meaningful once getJPClass() is non-null.
	 */
	jvalue getJValue();

	jvalue convert();

private:
	void resolveSlot();

public:
	JPMatch::Type type;
	JPConversion *conversion;
	JPJavaFrame *frame;
	PyObject *object;

	/**
	 * Private communication channel from a JPConversion's matches() to its
	 * own convert() -- the *only* mechanism a conversion has for carrying
	 * anything discovered while matching (e.g. which candidate class it
	 * resolved against) forward, so it doesn't have to be recomputed once
	 * the match is chosen. `JPMatch::convert()` calls `conversion->convert
	 * (*this)` with no other parameter (`jp_classhints.cpp`), so this field
	 * is genuinely the whole channel, not one option among several.
	 *
	 * Scoped per-conversion, not global: whatever a given matches()
	 * implementation stores here is a private contract with that exact
	 * class's own convert() -- never read by a different JPConversion, and
	 * every matches() that writes it must have a convert() that reads it
	 * back (a write with no corresponding read is dead code, not a
	 * harmless no-op -- one existed in this file for years before being
	 * caught; don't reintroduce that).
	 *
	 * Must stay non-owning (today: always a `JPClass*`/`JPFunctional*` the
	 * class registry owns for the program's lifetime, or a value that
	 * fits directly in the pointer, e.g. an integer -- never something
	 * this field is responsible for freeing). This is a hard constraint,
	 * not a style preference: `JPMethodDispatch::findOverload`
	 * (`jp_methoddispatch.cpp`) keeps two `JPMethodMatch` structs in
	 * flight per dispatch (`bestMatch`, and one `match` reused across
	 * every candidate tried) and copies one into the other via plain
	 * memberwise assignment whenever the best candidate changes --
	 * `JPMatch` has no custom copy constructor or destructor. A shallow
	 * pointer copy of a non-owning value is exactly correct under that;
	 * an owning pointer would silently leak the moment the best candidate
	 * improves more than once in one dispatch, since the old value is
	 * never explicitly freed on overwrite. If a conversion ever needs to
	 * pass real owned memory forward, it belongs inside that conversion's
	 * own convert() as a local, RAII-scoped to that one call -- not here.
	 */
	void *closure;

	/**
	 * Whether the current matches()/findJavaConversion() decision is
	 * determined purely by Py_TYPE(object) (and the JPClass being matched
	 * against), with no dependence on object's value/contents.
	 *
	 * Defaults to true (opt-out, not opt-in): a converter must explicitly
	 * clear this if its decision inspects the object itself (e.g. duck-typed
	 * attribute presence, or a sequence's element types) rather than just its
	 * type. Default-true is required for this to compose correctly across
	 * the few findJavaConversion implementations (JPBoxedType, JPFunctional)
	 * that call into another class's findJavaConversion as a building block:
	 * an opt-in ("only ever set true") flag would get silently reset by
	 * those nested calls, but a monotonic "only ever gets cleared" flag
	 * degrades safely instead.
	 *
	 * JPClass::findJavaConversion consults this after a cache miss to decide
	 * whether the resolved {conversion, type} pair is safe to memoize keyed
	 * on Py_TYPE(object) alone.
	 */
	bool cacheable;

private:
	bool m_SlotResolved;
	JPClass *m_SlotClass;
	jvalue m_SlotValue;
} ;

class JPMethodMatch
{
public:

	JPMethodMatch(JPJavaFrame &frame, JPPyObjectVector& args, bool callInstance);

	JPMatch& operator[](size_t i)
	{
		return m_Arguments[i];
	}

	const JPMatch& operator[](size_t i) const
	{
		return m_Arguments[i];
	}

	std::vector<JPMatch> m_Arguments;
	JPMatch::Type m_Type;
	bool m_IsVarIndirect;
	char m_Offset;
	char m_Skip;
	long m_Hash{-1};
	JPMethod* m_Overload{nullptr};
} ;

#endif /* JP_MATCH_H */
