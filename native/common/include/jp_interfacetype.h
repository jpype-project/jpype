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
#ifndef _JPINTERFACETYPE_H_
#define _JPINTERFACETYPE_H_

/**
 * Wrapper for interfaces, which can be targeted by a Python @JImplements
 * proxy -- unlike a plain (non-interface) class, which a dynamic proxy can
 * never be assigned to. Constructed instead of the plain JPClass base
 * whenever JPModifier::isInterface(modifiers) is true (see
 * TypeFactoryNative_defineObjectClass), so proxyConversion is only ever
 * tried against a class it could actually match -- JPClass's own
 * findJavaConversionImpl doesn't include it at all.
 */
class JPInterfaceType : public JPClass
{
public:
	JPInterfaceType(JPJavaFrame& frame, jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);

	~ JPInterfaceType() override;

	JPMatch::Type findJavaConversionImpl(JPMatch &match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

#endif // _JPINTERFACETYPE_H_
