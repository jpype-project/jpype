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
#ifndef _JPBUFFERTYPE_H_
#define _JPBUFFERTYPE_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPBufferType : public JPClass
{
public:
	JPBufferType(JPJavaFrame& frame, jclass cls, const string& name, JPClass* superClass, const JPClassList& interfaces, jint modifiers);
	~ JPBufferType() override;

	const char* getType()
	{
		return m_Type;
	}

	int getSize() const
	{
		return m_Size;
	}

	JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast) override;

private:
	const char* m_Type;
	int m_Size;
} ;

#endif // _JPBUFFERCLASS_H_