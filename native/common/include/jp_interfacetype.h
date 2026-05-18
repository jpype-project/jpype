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
 * Wrapper for all interfaces that can take proxies
 *
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

	JPMatch::Type findJavaConversion(JPMatch& match) override;
	void getConversionInfo(JPJavaFrame& frame, JPConversionInfo &info) override;
} ;

#endif // _JPOBJECTTYPE_H_
