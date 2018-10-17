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
#include <jpype.h>

JPMonitor::JPMonitor(const JPValue& value) : m_Value(value)
{
	// This can hold off for a while so we need to release resource 
	// so that we don't dead lock.
	JPPyCallRelease call;
	JPJavaFrame frame;
	jvalue& val = m_Value;
	frame.MonitorEnter(val.l);
	val.l = frame.NewGlobalRef(val.l);
}

JPMonitor::~JPMonitor()
{
	try
	{
		JPJavaFrame frame;
		jvalue& val = m_Value;
		frame.MonitorExit(val.l);
		frame.DeleteGlobalRef(val.l);
	} catch (...)
	{
		// Don't be silent.  Force it to our tracer log.
		JPypeTracer::trace("Exception in monitor destructor");
	}
}

JPValue& JPMonitor::getValue()
{
	return m_Value;
}
