/*****************************************************************************
   Copyright 2004 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 *****************************************************************************/
#ifndef JP_STRINGCLASS_H
#define JP_STRINGCLASS_H

class JPStringType : public JPClass
{
public:

	JPStringType(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);
	virtual ~JPStringType();

public:
	virtual JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val) override;
	JPMatch::Type getJavaConversion(JPJavaFrame* frame, JPMatch& match, PyObject* pyobj) override;
	virtual JPValue newInstance(JPJavaFrame& frame, JPPyObjectVector& args) override;
} ;

#endif /* JP_STRINGTYPE_H */
