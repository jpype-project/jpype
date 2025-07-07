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
	 * Get the Java slot associated with the Python object.
	 *
	 * Thus uses caching.
	 *
	 * @return the Java slot or 0 if not available.
	 */
	JPValue *getJavaSlot();

	jvalue convert();

public:
	JPMatch::Type type;
	JPConversion *conversion;
	JPJavaFrame *frame;
	PyObject *object;
	JPValue *slot;
	void *closure;
} ;

class JPMethodCache
{
public:
	long m_Hash;
	JPMethod* m_Overload;
} ;

class JPMethodMatch : public JPMethodCache
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
} ;

#endif /* JP_MATCH_H */
