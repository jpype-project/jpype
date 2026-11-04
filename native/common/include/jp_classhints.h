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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

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

	std::atomic<bool> m_ConvertJava{false};
private:
	using ConversionList = std::vector<JPConversion*>;

	using Snapshot = std::shared_ptr<const ConversionList>;

	/** Publish a new conversion list built from the current one.
	 *
	 * The list is read on every overload resolution, possibly while another
	 * thread registers a JConversion.  Readers take a reference-counted,
	 * immutable snapshot (an atomic shared_ptr load, which never runs Python
	 * code) and iterate it without any lock held; a replaced snapshot is
	 * freed when its last reader lets go.  Writers serialize on m_Lock, copy
	 * the current snapshot, apply their change and publish the copy.  The
	 * conversions themselves live as long as the hints (m_Owned), so a
	 * pointer from an old snapshot always stays valid.
	 */
	template <class Edit>
	void update(Edit edit);

	Snapshot snapshot() const
	{
		return std::atomic_load(&m_Conversions);
	}

	std::mutex m_Lock;
	std::vector<std::unique_ptr<JPConversion>> m_Owned; // owns every conversion, guarded by m_Lock
	Snapshot m_Conversions;                              // accessed only through std::atomic_load/store
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
extern JPConversion *boxLongConversion;
extern JPConversion *boxDoubleConversion;
extern JPConversion *unboxConversion;
extern JPConversion *proxyConversion;

#endif /* JP_CLASSHINTS_H */
