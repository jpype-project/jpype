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
#ifndef _JPBASECLASS_H_
#define _JPBASECLASS_H_

/**
 * Wrapper for Class<java.lang.Object>
 *
 * Primitive types can implicitely cast to this type as
 * well as class wrappers, thus we need a specialized
 * wrapper.  This class should not be used outside of
 * the JPTypeManager.
 *
 */
class JPObjectBaseClass : public JPClass
{
public:
	JPObjectBaseClass();
	virtual~ JPObjectBaseClass();

public: // JPClass implementation
	virtual JPMatch::Type canConvertToJava(PyObject* obj) override;
	virtual jvalue     convertToJava(PyObject* obj) override;
} ;

/**
 * Wrapper for Class<java.lang.Class>
 *
 * Class wrappers need to be able to cast to this type,
 * thus we need a specialized version.
 * This class should not be used outside of
 * the JPTypeManager.
 */
class JPClassBaseClass : public JPClass
{
public:
	JPClassBaseClass();
	virtual~ JPClassBaseClass();

public: // JPClass implementation
	virtual JPMatch::Type canConvertToJava(PyObject* obj) override;
	virtual jvalue     convertToJava(PyObject* obj) override;
} ;

#endif // _JPBASECLASS_H_
