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
#ifndef JP_CLASSHINTS_H
#define JP_CLASSHINTS_H

class JPConversion
{
public:
	virtual ~JPConversion();

	virtual JPMatch::Type matches(JPClass *cls, JPMatch &match) = 0;

	virtual void getInfo(JPClass *cls, JPConversionInfo &info) = 0;

	virtual jvalue convert(JPMatch &match) = 0;
} ;

class JPIndexConversion : public JPConversion
{
public:

	void getInfo(JPClass *cls, JPConversionInfo &info) override;

} ;

class JPNumberConversion : public JPIndexConversion
{
public:

	void getInfo(JPClass *cls, JPConversionInfo &info) override;

} ;

class JPConversionJavaValue : public JPConversion
{
public:
	JPMatch::Type matches(JPClass *cls, JPMatch &match) override;
	void getInfo(JPClass *cls, JPConversionInfo &info) override;
	jvalue convert(JPMatch &match) override;
} ;

class JPClassHints
{
public:
	JPClassHints();
	~JPClassHints();

	/** Get the conversion of this type.
	 *
	 * Searches the list for a conversion. The first conversion better than
	 * explicit is returned immediately.
	 *
	 * @returns the quality of the match
	 */
	JPMatch::Type getConversion(JPMatch& match, JPClass *cls);

	/**
	 * Add a conversion based on a specified attribute.
	 *
	 * If the attribute is found in in the object, it is assumed to be a match.
	 * This is for "duck type" conversions.  The Python routine must return
	 * something that holds a __javavalue__ which will be used as the converted
	 * object.
	 *
	 * @param attribute is the attribute to search for.
	 * @param method is a Python routine to call to complete the conversion
	 * process.
	 */
	void addAttributeConversion(const string& attribute, PyObject* method);

	/**
	 * Add a type conversion based on the Python type.
	 *
	 * The Python routine must return something that holds a __javavalue__
	 * which will be used as the converted object.
	 *
	 * @param type is a Python type object
	 * @param method is a Python routine to call to complete the conversion
	 * process.
	 * @param exact require the type to be an exact match.
	 */
	void addTypeConversion(PyObject* type, PyObject* method, bool exact);

	void excludeConversion(PyObject* type);

	void getInfo(JPClass *cls, JPConversionInfo &info);

	bool m_ConvertJava;

	/**
	 * Bumped every time any JPClassHints anywhere gains a new conversion
	 * (addTypeConversion/addAttributeConversion/excludeConversion).
	 *
	 * JPClass::findJavaConversion's per-type cache compares its own stashed
	 * generation against this on every call and clears itself if stale. This
	 * is deliberately global/coarse rather than per-owning-class (which
	 * would need a back-pointer this class doesn't have) -- hint mutation
	 * only happens at setup time, so invalidating a bit more than strictly
	 * necessary costs nothing in practice.
	 */
	static uint64_t s_Generation;
private:
	std::list<JPConversion*> conversions;
} ;

extern JPConversion *hintsConversion;
extern JPConversion *charArrayConversion;
extern JPConversion *byteArrayConversion;
extern JPConversion *bufferConversion;
extern JPConversion *sequenceConversion;
extern JPConversion *nullConversion;
extern JPConversion *classConversion;
extern JPConversion *objectConversion;
extern JPConversion *javaObjectAnyConversion;
extern JPConversion *javaNumberAnyConversion;
extern JPConversion *javaValueConversion;
extern JPConversion *stringConversion;
extern JPConversion *boxConversion;
extern JPConversion *boxBooleanConversion;
extern JPConversion *boxGenericConversion;
extern JPConversion *boxLongConversion;
extern JPConversion *boxDoubleConversion;
extern JPConversion *unboxConversion;
extern JPConversion *proxyConversion;

#endif /* JP_CLASSHINTS_H */
