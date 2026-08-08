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
	void *closure;

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
