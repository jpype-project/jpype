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
#include <jpype.h>

jobject JPValue::getJavaObject() const
{
	// This is all sanity check
	// GCOVR_EXCL_START
	if (m_Class == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null class");
	if (!m_Class->isPrimitive())
		// GCOVR_EXCL_STOP
		return m_Value.l;

	// This method is only used internally, thus it requires a logical code
	// error to trigger. We will use type error in case there is some
	// way a user can trigger it.
	JP_RAISE(PyExc_TypeError, "cannot access Java primitive value as Java object");  // GCOVR_EXCL_LINE
}
