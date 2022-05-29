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
#ifndef _JPCLASSTYPE_H_
#define _JPCLASSTYPE_H_

/**
 * Wrapper for Class<java.lang.Class>
 *
 * Class wrappers need to be able to cast to this type,
 * thus we need a specialized version.
 * This class should not be used outside of
 * the JPTypeManager.
 */
class JPClassType : public JPClass
{
public:
	JPClassType(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);

	~ JPClassType() override;

public: // JPClass implementation
	JPMatch::Type findJavaConversion(JPMatch &match) override;
	void getConversionInfo(JPConversionInfo &info) override;

} ;

#endif // _JPCLASSTYPE_H_