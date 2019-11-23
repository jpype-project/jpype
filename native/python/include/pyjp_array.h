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
#ifndef _PYJP_ARRAY_H_
#define _PYJP_ARRAY_H_

extern PyObject* PyJPArray_Type;
/** This is a wrapper for accessing the array method.  It is structured to
 * be like a bound method.  It should not be the primary handle to the object.
 * That will be a PyJPValue.
 */
struct PyJPArray
{
	PyJPValue m_Value;
	JPArray *m_Array;

	static void initType(PyObject *module);
} ;

#endif // _PYJP_ARRAY_H_
